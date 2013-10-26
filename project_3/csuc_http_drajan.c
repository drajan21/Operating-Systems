/******************************************************************************************************************************************

FILE NAME: csuc_http_drajan.c
STUDENT NAME:Diana Rajan
COURSE: CSCI 640
PROF. NAME: CHRIS MORRIS

******************************************************************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "csuc_http.h"
#include <fcntl.h>
#include <unistd.h>


#define MEMORYSIZE                 1000
#define INVALID_METHOD             -1
#define POSITION_OK                2
#define BACKLOG                    10
#define POSITION_BADREQUEST        16
#define POSITION_NOTFOUND          20
#define POSITION_INTERNALERROR     34
#define POSITION_NOTIMPLEMENTED    35
#define POSITION_HTTPVERSION       39
#define POSITION_SERVER            3
#define POSITION_DATE              2
#define POSITION_CONTENTTYPE       4
#define POSITION_CONTENTLENGTH     5
#define SIZE                       1000000
#define ZOMBIECHILD                -1
#define ERROR                      -1
#define FILENOTFOUND               "FileNotFound.html"
#define FILEINTERNALERROR          "FileInternalError.html"
#define FILENOTIMPLEMENTED         "FileNotImplemented.html"
#define FILEHTTPVERSION            "FileHTTPVersionNotSuuported.html"


/* Function to display datetime */
int getdatetime(http_response_t *http_response)
{
	time_t rawtime;
        struct tm *info;
        char datetime[100];
        time( &rawtime );
        info = gmtime( &rawtime );

        size_t bytes=strftime(datetime,80,"%a, %d %b %Y %X GMT", info); 
	strncpy(http_response->headers[POSITION_DATE].field_name,"Date",MAX_HEADER_NAME_LENGTH);
        strncpy(http_response->headers[POSITION_DATE].field_value,datetime,MAX_HEADER_VALUE_LENGTH);
	return 0;
}


void sendresponse(int nsockfd,http_response_t http_response,char *fullfilepath)
{
	char readfilebuf[SIZE],writefilebuf[SIZE];
	char * responseheader=malloc(MEMORYSIZE);
	char * responsestatus=malloc(MEMORYSIZE);
	int file_fd;
	ssize_t numbytes;

	sprintf(responsestatus,"HTTP/%d.%d %d %s\r\n",http_response.major_version,http_response.minor_version,http_response.status.code,http_response.status.reason);
 
        sprintf(responseheader,"%s: %s\r\n%s: %s\r\n%s: %s\r\n%s: %s\r\n\r\n",
				http_response.headers[POSITION_DATE].field_name,http_response.headers[POSITION_DATE].field_value,
				http_response.headers[POSITION_SERVER].field_name,http_response.headers[POSITION_SERVER].field_value,
				http_response.headers[POSITION_CONTENTTYPE].field_name,http_response.headers[POSITION_CONTENTTYPE].field_value,
				http_response.headers[POSITION_CONTENTLENGTH].field_name,http_response.headers[POSITION_CONTENTLENGTH].field_value);
  
	if(http_response.status.code==HTTP_STATUS_LOOKUP[POSITION_NOTFOUND].code)
	{
		file_fd=open(FILENOTFOUND,O_RDONLY,S_IRUSR);
	}
	else if(http_response.status.code==HTTP_STATUS_LOOKUP[POSITION_NOTIMPLEMENTED].code)
        {
                file_fd=open(FILENOTIMPLEMENTED,O_RDONLY,S_IRUSR);
        }
	else if(http_response.status.code==HTTP_STATUS_LOOKUP[POSITION_INTERNALERROR].code)
        {
                file_fd=open(FILEINTERNALERROR,O_RDONLY,S_IRUSR);
        }
	else if(http_response.status.code==HTTP_STATUS_LOOKUP[POSITION_HTTPVERSION].code)
        {
                file_fd=open(FILEHTTPVERSION,O_RDONLY,S_IRUSR);
        }
	else
	{
		file_fd=open(fullfilepath,O_RDONLY,S_IRUSR);	
	}
	numbytes=read(file_fd,readfilebuf,SIZE);
	numbytes=write(nsockfd,responsestatus,strlen(responsestatus));
	if(numbytes==ERROR)
	{
		perror("Write to Socket Error");
	}
	numbytes=write(nsockfd,responseheader,strlen(responseheader));
	if(numbytes==ERROR)
        {
                perror("Write to Socket Error");
        }

	numbytes=write(nsockfd,readfilebuf,SIZE);
	if(numbytes==ERROR)
        {
                perror("Write to Socket Error");
        }

	sleep(1);
	free(responseheader);
	free(responsestatus);
	close(file_fd);
}

/*Function to parse path and obtain file type */
char * parsepath(char *path)
{
        char *type;
        type="UNKOWN FILE TYPE";
	type=strrchr(path,'.');
        if(strcmp(type,".html")==0)
                type="text/html";
        if(strcmp(type,".jpeg")==0)
                type="image/jpeg";
        if(strcmp(type,".png")==0)
                type="image/png";
	if(strcmp(type,".css")==0)
                type="text/css";
        if(strcmp(type,".js")==0)
                type="application/javascript";
        if(strcmp(type,".xml")==0)
                type="application/xml";

        return type;
}

/*Function to find the content length*/
int find_content_length(char *filepath, http_status_t *http_status)
{
         struct stat stfile;
	 int content_length=0;
	 FILE *fpread=fopen(filepath,"r");
	 if(fpread==NULL)
         {
         	http_status->code=HTTP_STATUS_LOOKUP[POSITION_NOTFOUND].code;
                http_status->reason=HTTP_STATUS_LOOKUP[POSITION_NOTFOUND].reason;
	 }
         else
         {
         	int statcheck=stat(filepath,&stfile);
		if(statcheck==0)
		{
                	content_length=stfile.st_size;
		}
		else
		{
                	http_status->code=HTTP_STATUS_LOOKUP[POSITION_INTERNALERROR].code;
                	http_status->reason=HTTP_STATUS_LOOKUP[POSITION_INTERNALERROR].reason;
		}
	}
	fclose(fpread);
	return content_length;
}
 
/* Function to Build a response */
int buildresponse(http_request_t *http_request,http_status_t *http_status,http_response_t *http_response)
{
	int file_fd;
	ssize_t numread;
	char readbuf[MEMORYSIZE],writebuf[MEMORYSIZE];
	
	http_header_t http_response_header;
	int headerindex=0;
	int position;
	char *uripath;

	uripath=strdup(http_request->uri);

	getdatetime(http_response);
	headerindex++;

	strncpy(http_response->resource_path,http_request->uri,PATH_MAX);

        char *contenttype;
        if(uripath)
        	contenttype=parsepath(uripath);

	strncpy(http_response->headers[POSITION_SERVER].field_name,"Server",MAX_HEADER_NAME_LENGTH);
	strncpy(http_response->headers[POSITION_SERVER].field_value,"CSUC",MAX_HEADER_VALUE_LENGTH);
	headerindex++;

	http_response->status.code=http_status->code;
	http_response->status.reason=http_status->reason;

	http_response->major_version=http_request->major_version;
	http_response->minor_version=http_request->minor_version;
	
	strncpy(http_response->headers[POSITION_CONTENTTYPE].field_name,"Content-Type",MAX_HEADER_NAME_LENGTH);
	strncpy(http_response->headers[POSITION_CONTENTTYPE].field_value,contenttype,MAX_HEADER_VALUE_LENGTH);
        headerindex++;

	strncpy(http_response->headers[POSITION_CONTENTLENGTH].field_name,"Content-Length",MAX_HEADER_NAME_LENGTH);
	char lengthbuffer[MEMORYSIZE];
	int writecheck;
	if(http_status->code==HTTP_STATUS_LOOKUP[POSITION_NOTFOUND].code)
	{
		file_fd=open(FILENOTFOUND,O_CREAT|O_RDWR,S_IRUSR|S_IWUSR);
		if(file_fd==-1)
			perror("File Open Error");
		else
		{
			numread=read(file_fd,readbuf,MEMORYSIZE);
			if(numread==0)
			{
				sprintf(writebuf,"404 Error-Page Not Found");
				writecheck=write(file_fd,writebuf,strlen(writebuf));
				if(writecheck==-1)
					perror("File Write Error");
				else
				{
					sprintf(lengthbuffer,"%d",strlen(writebuf));
					strncpy(http_response->headers[POSITION_CONTENTLENGTH].field_value,lengthbuffer,MAX_HEADER_VALUE_LENGTH);
                			headerindex++;
				}
			}
			else
			{
				sprintf(lengthbuffer,"%d",numread);
                        	strncpy(http_response->headers[POSITION_CONTENTLENGTH].field_value,lengthbuffer,MAX_HEADER_VALUE_LENGTH);
                        	headerindex++;
			}
		}
		close(file_fd);
	}
	else if(http_status->code==HTTP_STATUS_LOOKUP[POSITION_INTERNALERROR].code)
        {
                file_fd=open(FILEINTERNALERROR,O_CREAT|O_RDWR,S_IRUSR|S_IWUSR);
		 if(file_fd==-1)
                        perror("File Open Error");
                else
                {
                	numread=read(file_fd,readbuf,MEMORYSIZE);
                	if(numread==0)
                	{
                        	sprintf(writebuf,"500 Error-Internal Server Error");
                        	writecheck=write(file_fd,writebuf,strlen(writebuf));
				if(writecheck==-1)
                                        perror("File Write Error");
                                else
                                {

                        		sprintf(lengthbuffer,"%d",strlen(writebuf));
                        		strncpy(http_response->headers[POSITION_CONTENTLENGTH].field_value,lengthbuffer,MAX_HEADER_VALUE_LENGTH);
                        		headerindex++;
				}
                	}
                	else
                	{
                        	sprintf(lengthbuffer,"%d",numread);
                        	strncpy(http_response->headers[POSITION_CONTENTLENGTH].field_value,lengthbuffer,MAX_HEADER_VALUE_LENGTH);
                        	headerindex++;
                	}
		}
                close(file_fd);
        }
	else if(http_status->code==HTTP_STATUS_LOOKUP[POSITION_NOTIMPLEMENTED].code)
        {
                file_fd=open(FILENOTIMPLEMENTED,O_CREAT|O_RDWR,S_IRUSR|S_IWUSR);
		if(file_fd==-1)
                        perror("File Open Error");
                else
                {

                	numread=read(file_fd,readbuf,MEMORYSIZE);
               		if(numread==0)
                	{
                        	sprintf(writebuf,"501 Error-Not Implemented");
                        	write(file_fd,writebuf,strlen(writebuf));
				if(writecheck==-1)
                                        perror("File Write Error");
                                else
                                {
                        		sprintf(lengthbuffer,"%d",strlen(writebuf));
                        		strncpy(http_response->headers[POSITION_CONTENTLENGTH].field_value,lengthbuffer,MAX_HEADER_VALUE_LENGTH);
                        		headerindex++;
				}
                	}
                	else
                	{
                        	sprintf(lengthbuffer,"%d",numread);
                        	strncpy(http_response->headers[POSITION_CONTENTLENGTH].field_value,lengthbuffer,MAX_HEADER_VALUE_LENGTH);
                        	headerindex++;
                	}
		}
                close(file_fd);
        }
	 if(http_status->code==HTTP_STATUS_LOOKUP[POSITION_HTTPVERSION].code)
        {
                file_fd=open(FILEHTTPVERSION,O_CREAT|O_RDWR,S_IRUSR|S_IWUSR);
		if(file_fd==-1)
                        perror("File Open Error");
                else
                {
                	numread=read(file_fd,readbuf,MEMORYSIZE);
                	if(numread==0)
                	{
                        	sprintf(writebuf,"505 Error-HTTP Version Not Supported");
                        	write(file_fd,writebuf,strlen(writebuf));
				if(writecheck==-1)
                                        perror("File Write Error");
                                else
                                {
                        		sprintf(lengthbuffer,"%d",strlen(writebuf));
                        		strncpy(http_response->headers[POSITION_CONTENTLENGTH].field_value,lengthbuffer,MAX_HEADER_VALUE_LENGTH);
                        		headerindex++;
				}
                	}
                	else
                	{
                        	sprintf(lengthbuffer,"%d",numread);
                        	strncpy(http_response->headers[POSITION_CONTENTLENGTH].field_value,lengthbuffer,MAX_HEADER_VALUE_LENGTH);
                        	headerindex++;
                	}
		}
                close(file_fd);
        }
	else if(http_status->code==HTTP_STATUS_LOOKUP[POSITION_OK].code)
	{
		int length;
		strncpy(http_response->headers[POSITION_CONTENTLENGTH].field_name,"Content-Length",MAX_HEADER_NAME_LENGTH);
		length=find_content_length(http_request->uri,http_status);
		sprintf(lengthbuffer,"%d",length);
		strncpy(http_response->headers[POSITION_CONTENTLENGTH].field_value,lengthbuffer,MAX_HEADER_VALUE_LENGTH);
		headerindex++;
	}
	http_response->header_count=headerindex;

	free(uripath);
	return;
}

/* Function to parse the Headers */
int preparerequestheader(const char *headerline,int headerindex,http_request_t *http_request)
{
	int returnval=0;
	char firstchar;
	char *header=strdup(headerline);
	char *remaining;
	char *tok=strtok_r(header,":",&remaining);

	if(tok)
	{
		strcpy(http_request->headers[headerindex].field_name,tok);
		strcpy(http_request->headers[headerindex].field_value,remaining);
		
		int field_val_length=strlen(http_request->headers[headerindex].field_value);
		firstchar=http_request->headers[headerindex].field_value[0];
		if(firstchar==' ')
			/* Moves pointer by 1 position */
			memmove(http_request->headers[headerindex].field_value,http_request->headers[headerindex].field_value+1,field_val_length);
		returnval=1;
	}
	else
		returnval=0;
	return returnval;
}


http_method_t parseverb(char *method)
{

	http_method_t http_method;
		http_method=INVALID_METHOD;	
	if(strcmp(method,"GET")==0)
		http_method= HTTP_METHOD_GET;
	if(strcmp(method,"OPTIONS")==0)
                http_method= HTTP_METHOD_OPTIONS;
	if(strcmp(method,"HEAD")==0)
                http_method= HTTP_METHOD_HEAD;
        if(strcmp(method,"POST")==0)
                http_method= HTTP_METHOD_POST;
	 if(strcmp(method,"PUT")==0)
                http_method= HTTP_METHOD_PUT;
        if(strcmp(method,"DELETE")==0)
                http_method= HTTP_METHOD_DELETE;
	 if(strcmp(method,"TRACE")==0)
                http_method= HTTP_METHOD_TRACE;
        if(strcmp(method,"CONNECT")==0)
                http_method= HTTP_METHOD_CONNECT;

	return http_method;
}

/*Function checks for #/? in the path */
char * returnpath(char * uri_path)
{
	char *final_path=malloc(MEMORYSIZE);
	char *check;
	check=strchr(uri_path,'#');
	if(check!=NULL)
		sscanf(uri_path,"%[^#]",final_path);
	else
		strncpy(final_path,uri_path,strlen(uri_path));
	
	check=NULL;
	check=strchr(uri_path,'?');
	if(check!=NULL)
		sscanf(uri_path,"%[^?]",final_path);
	else
		 strncpy(final_path,uri_path,strlen(uri_path));

	return final_path;
}

void process_request(char *read_request,char *site,int newsockfd)
{
	int majversion,minversion;

	char *verb;
        verb=(char *)malloc(MEMORYSIZE);

        char *path;
        path=(char *)malloc(MEMORYSIZE);

        char *version;
        version=(char *)malloc(MEMORYSIZE);

	char *header_name,*header_value;
	header_name=(char *)malloc(MEMORYSIZE);
	header_value=(char *)malloc(MEMORYSIZE);
	
	sscanf(read_request,"%s %s %s",verb,path,version);
	sscanf(version,"HTTP/%d.%d",&majversion,&minversion);	

	http_method_t http_method;
	http_request_t http_request;
	http_response_t http_response;
        http_status_t http_status;

	char *filetype;

	http_method=parseverb(verb);

	if(strchr(path,'.')==NULL)
	{
		if(path[strlen(path)-1]!='/')
			strcat(path,"/index.html");
		else
			strcat(path,"index.html");
	}
	/* Checks for #/? */
	path=returnpath(path);
	/* Gets the filetype */
	filetype=parsepath(path);

	int headercount=0,parseheaderval=0,headerline_length=0;
	char *read_header;
	read_header=(char *)malloc(MEMORYSIZE);

	read_request=read_request+(strlen(verb)+strlen(path)+strlen(version)+4);
	/* Saving request headers */
	while(headercount!=MAX_HEADERS)
	{
		if(*(read_request+0)==13 && *(read_request+1)==10)
			break;

		sscanf(read_request,"%[^\n]",read_header);
                headerline_length=strlen(read_header);
		parseheaderval= preparerequestheader(read_header,headercount,&http_request);
		headercount++;
		memset(read_header,'0',strlen(read_header));
		read_request=read_request+headerline_length+1;
	}

	http_request.method=http_method;
	http_request.major_version=majversion;
        http_request.minor_version=minversion;
        http_request.header_count=headercount;

	char *fullfilepath=malloc(1000);
        strcat(fullfilepath,site);
        strcat(fullfilepath,path);
	strncpy(http_request.uri,fullfilepath,URI_MAX);

	if(http_request.method!=HTTP_METHOD_GET)
	{
		http_status.code=HTTP_STATUS_LOOKUP[POSITION_NOTIMPLEMENTED].code;
                http_status.reason=HTTP_STATUS_LOOKUP[POSITION_NOTIMPLEMENTED].reason;
	}
	else if(http_request.method==HTTP_METHOD_GET)
    	{
        	DIR *dir;
                FILE *fpread;
                struct stat stfile;
                dir=opendir(site);

                if(dir==NULL)
                {
                	http_status.code=HTTP_STATUS_LOOKUP[POSITION_NOTFOUND].code;
                        http_status.reason=HTTP_STATUS_LOOKUP[POSITION_NOTFOUND].reason;
			buildresponse(&http_request,&http_status,&http_response);
                        sleep(1);
                        sendresponse(newsockfd,http_response,fullfilepath);

                }

                else
                {
                 	FILE *fpread=fopen(fullfilepath,"r");
			if(fpread==NULL)
                        {
                        	http_status.code=HTTP_STATUS_LOOKUP[POSITION_NOTFOUND].code;
                                http_status.reason=HTTP_STATUS_LOOKUP[POSITION_NOTFOUND].reason;
				buildresponse(&http_request,&http_status,&http_response);
			        sleep(1);
        			sendresponse(newsockfd,http_response,fullfilepath);
                        }
                        else
                        {
				            http_status.code=HTTP_STATUS_LOOKUP[POSITION_OK].code;
                        	http_status.reason=HTTP_STATUS_LOOKUP[POSITION_OK].reason;
				buildresponse(&http_request,&http_status,&http_response);
			        sleep(1);
        			sendresponse(newsockfd,http_response,fullfilepath);

		        }
			        fclose(fpread);
		 }
	}

	free(fullfilepath);
	free(read_header);
	free(header_name);
	free(header_value);
	free(version);
	free(path);
	free(verb);
}

int main(int argc, char * argv[])
{

	int sockfd,newsockfd;
	struct sockaddr_in serv_addr;
	ssize_t bytes;
	char readbuf[SIZE];
	char *read_request=malloc(SIZE);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
    	if(sockfd==ERROR)
	{
		perror("Socket Creation Error");
	}
	memset(&serv_addr, '0', sizeof(serv_addr));
    	
    	serv_addr.sin_family = AF_INET;
    	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    	serv_addr.sin_port = htons(atol(argv[2]));

    	int bindreturn=bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
	if(bindreturn==ERROR)
	{
		perror("Socket Bind Error");
	}

    	int listenreturn=listen(sockfd, BACKLOG);
	if(listenreturn==ERROR)
	{
		perror("Socket Listen Error");
	}
	pid_t childid;
	while(1)
	{
		newsockfd=accept(sockfd,(struct sockaddr*)NULL,NULL);
		if(newsockfd==ERROR)
		{
			perror("Socket Accept Error");
		}
		childid=fork();
		if(childid==ERROR)
		{
			perror("Fork Error");
		}
		else if(childid==0)
		{
			bytes=read(newsockfd,readbuf,SIZE);
	        	strncpy(read_request,readbuf,SIZE);
			process_request(read_request,argv[1],newsockfd);
			close(newsockfd);
			free(read_request);
			return(0);
		}
		else
		{
			close(newsockfd);
			/* Killing Zombies */
			while(waitpid(ZOMBIECHILD,0,WNOHANG)!=0);
		}
	}
	free(read_request);
	close(sockfd);
	return(0);
}
