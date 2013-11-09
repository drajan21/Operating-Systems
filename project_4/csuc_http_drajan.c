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
#include <pthread.h>

#define MEMORYSIZE                 1000
#define INVALID_METHOD             -1
#define POSITION_OK                2
#define BACKLOG                    10
#define POSITION_BADREQUEST        16
#define POSITION_NOTFOUND          20
#define POSITION_INTERNALERROR     34
#define POSITION_NOTIMPLEMENTED    35
#define POSITION_SERVER            3
#define POSITION_DATE              2
#define POSITION_CONTENTTYPE       4
#define POSITION_CONTENTLENGTH     5
#define SIZE                       1000000
#define ZOMBIECHILD                -1
#define ERROR                      -1
#define FILENOTFOUND               "404.html"
#define FILEINTERNALERROR          "r.html"
#define FILENOTIMPLEMENTED         "501.html"

char directory_name[MEMORYSIZE];

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

int sendcontent(char *path[], int newfp)
{

    FILE *file; int fd;
    int f_size;
    ssize_t wbytes;
    char *buffer = malloc(sizeof(char) * SIZE);
    memset(buffer,0, sizeof(buffer));
    if(access(*path,F_OK)==0)
    {
        file = fopen(path[0], "r");
        if (file)
        {
            while((f_size= fread(buffer,1,sizeof(buffer),file)) > 0)
            wbytes=write(newfp,buffer,f_size);
        }
        else
        {
            perror("Error Opening File");
        }
        fclose(file);
    }
    else
        perror(NULL);
   
    free(buffer);
    close(newfp);
    return 0;
}

char *getfilepath(int status)
{
    FILE * fp;

    char *pathname=malloc(sizeof(char) * MEMORYSIZE);
    char *path404=malloc(sizeof(char) * MEMORYSIZE);
    char *path400=malloc(sizeof(char) * MEMORYSIZE);

    strcpy(path404,directory_name);
    strcat(path404,"/404.html");

    strcpy(path400,directory_name);
    strcat(path400,"/400.html");

    if(status==HTTP_STATUS_LOOKUP[POSITION_NOTFOUND].code)
    {
        fp=fopen(path404,"r");
        if(!fp)
        {
            fp=fopen(path400,"r");
            if(!fp)
                strcpy(pathname,"404.html");
            else
                strcpy(pathname,path400);
        }
        else
            strcpy(pathname,path404);
    }

    free(path404);
    free(path400);

    return pathname;

}

int sendresponse(int nsockfd,http_response_t http_response,char *fpath)
{
	char responseheader[MEMORYSIZE];
    memset(responseheader,0,sizeof(char)* MEMORYSIZE);
 
    char responsestatus[MEMORYSIZE];
	memset(responsestatus,0,sizeof(char) * MEMORYSIZE);

    char *filepath=malloc(sizeof(char) * MEMORYSIZE);

    int file_fd;
	ssize_t numbytes;

    sprintf(responsestatus,"HTTP/%d.%d %d %s\r\n",http_response.major_version,http_response.minor_version,http_response.status.code,http_response.status.reason);
    sprintf(responseheader,"%s: %s\r\n%s: %s\r\n%s: %s\r\n%s: %s\r\n\r\n",
	http_response.headers[POSITION_DATE].field_name,http_response.headers[POSITION_DATE].field_value,
	http_response.headers[POSITION_SERVER].field_name,http_response.headers[POSITION_SERVER].field_value,
	http_response.headers[POSITION_CONTENTTYPE].field_name,http_response.headers[POSITION_CONTENTTYPE].field_value,
	http_response.headers[POSITION_CONTENTLENGTH].field_name,http_response.headers[POSITION_CONTENTLENGTH].field_value);
     
    if(http_response.status.code!=HTTP_STATUS_LOOKUP[POSITION_OK].code)
	{
	    filepath=getfilepath(http_response.status.code);
	}

	else
	{
		strcpy(filepath,fpath);;	
	}
   
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

    sendcontent(&filepath,nsockfd);
    return 0;
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
    if(strcmp(type,".mp3")==0)
        type="audio/mpeg";
    if(strcmp(type,".mpeg")==0)
            type="video/mpeg";
    if(strcmp(type,".mpg")==0)
        type="video/mpg";
    if(strcmp(type,".mp4")==0)
        type="video/mp4";
    if(strcmp(type,".mov")==0)
        type="video/quicktime";

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

int checkforerrorfiles(http_status_t *status,http_response_t *http_response)
{
    FILE *fpcheck;
    int file_fd,writecheck;
    ssize_t numread;
    int checkfile=1,length;
    char *filename404=malloc(sizeof(char) * MEMORYSIZE);
    char *filename400=malloc(sizeof(char) * MEMORYSIZE);

    char readbuf[MEMORYSIZE],writebuf[MEMORYSIZE],lengthbuffer[MEMORYSIZE];
    strcpy(filename404,directory_name);
    strcat(filename404,"/404.html");

    strcpy(filename400,directory_name);
    strcat(filename400,"/400.html");

    switch(status->code)
    {
        case 404:
                fpcheck=fopen(filename404,"r");
                if(fpcheck==NULL)
                {
                    fpcheck=fopen(filename400,"r");
                    if(fpcheck==NULL)
                        checkfile=-1;    
                    else
                        length=find_content_length(filename400,status);
                }
                else
                    length=find_content_length(filename404,status);
                sprintf(lengthbuffer,"%d",length);
                strncpy(http_response->headers[POSITION_CONTENTLENGTH].field_value,lengthbuffer,MAX_HEADER_VALUE_LENGTH);
                break;
    
    }

    if(checkfile==-1)
    {
        switch(status->code)
        {
            case 404:
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
                        }
                    }
                    else
                        sprintf(lengthbuffer,"%d",numread);
                 }
        }
        strncpy(http_response->headers[POSITION_CONTENTLENGTH].field_value,lengthbuffer,MAX_HEADER_VALUE_LENGTH);
    }

    return checkfile;
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
    headerindex++;

	strncpy(http_response->headers[POSITION_CONTENTLENGTH].field_name,"Content-Length",MAX_HEADER_NAME_LENGTH);
	
    char lengthbuffer[MEMORYSIZE];
	int writecheck;
    if(http_status->code!=HTTP_STATUS_LOOKUP[POSITION_OK].code)
    {
        writecheck=checkforerrorfiles(http_status,http_response);
        strncpy(http_response->headers[POSITION_CONTENTTYPE].field_value,".html",MAX_HEADER_VALUE_LENGTH);

    }     
	else 
	{
	    int length;
        strncpy(http_response->headers[POSITION_CONTENTTYPE].field_value,contenttype,MAX_HEADER_VALUE_LENGTH);
		strncpy(http_response->headers[POSITION_CONTENTLENGTH].field_name,"Content-Length",MAX_HEADER_NAME_LENGTH);
		length=find_content_length(http_request->uri,http_status);
		sprintf(lengthbuffer,"%d",length);
		strncpy(http_response->headers[POSITION_CONTENTLENGTH].field_value,lengthbuffer,MAX_HEADER_VALUE_LENGTH);
	}
    headerindex++;
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
	free(header);
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


int process_path(char *path, char *site,http_request_t *http_request)
{
    memset(http_request->uri,0,URI_MAX);
    if(strchr(path,'.')==NULL)
    {
       if(path[strlen(path)-1]!='/')
           strcat(path,"/index.html");
       else
           strcat(path,"index.html");
    }

    char *check;
    char final_path[MEMORYSIZE];
    memset(final_path,0,sizeof(final_path));
    check=strchr(path,'#');
    if(check!=NULL)
        sscanf(path,"%[^#]",final_path);
    else
        strncpy(final_path,path,strlen(path));

    check=NULL;
    check=strchr(path,'?');
    if(check!=NULL)
        sscanf(path,"%[^?]",final_path);
    else
        strncpy(final_path,path,strlen(path));
    
    strncpy(http_request->uri,site,strlen(site));
    strcat(http_request->uri,final_path);
   
    return 0;
}

int reset_reponse_headers(http_response_t *http_response)
{
    int counter;
    for(counter=0;counter<4;counter++)
    {
        strcpy(http_response->headers[counter].field_name,"");
        strcpy(http_response->headers[counter].field_value,"");
    }
    http_response->header_count=0; 
    
    return 0;
}

int process_request(char *read_request,char *site,int newsockfd)
{

     FILE *fpread;
 
	 int majversion,minversion;
     http_method_t http_method;
     http_request_t http_request;
     http_response_t http_response;
     http_status_t http_status;

	char *verb;
    verb=(char *)malloc(MEMORYSIZE);

    char path[MEMORYSIZE];
    memset(path,0,sizeof(path));

    char *version;
    version=(char *)malloc(MEMORYSIZE);

	char *header_name,*header_value;
	header_name=(char *)malloc(MEMORYSIZE);
	header_value=(char *)malloc(MEMORYSIZE);
	
	sscanf(read_request,"%s %s %s",verb,path,version);
	sscanf(version,"HTTP/%d.%d",&http_request.major_version,&http_request.minor_version);	
    
    if(strcmp(path,"/favicon.ico")==0)
        return 0;
    
	http_method=parseverb(verb);

    process_path(path,site,&http_request);


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
    http_request.header_count=headercount;

	if(http_request.method!=HTTP_METHOD_GET)
	{
		http_status.code=HTTP_STATUS_LOOKUP[POSITION_NOTIMPLEMENTED].code;
        http_status.reason=HTTP_STATUS_LOOKUP[POSITION_NOTIMPLEMENTED].reason;
	}
	else if(http_request.method==HTTP_METHOD_GET)
    {
              
        fpread=fopen(http_request.uri,"r");
		if(!fpread)
        {
            http_status.code=HTTP_STATUS_LOOKUP[POSITION_NOTFOUND].code;
            http_status.reason=HTTP_STATUS_LOOKUP[POSITION_NOTFOUND].reason;
	        buildresponse(&http_request,&http_status,&http_response);
			sendresponse(newsockfd,http_response,http_request.uri);
            reset_reponse_headers(&http_response);
        }
        else
        {
		    http_status.code=HTTP_STATUS_LOOKUP[POSITION_OK].code;
            http_status.reason=HTTP_STATUS_LOOKUP[POSITION_OK].reason;
		    buildresponse(&http_request,&http_status,&http_response);
			sendresponse(newsockfd,http_response,http_request.uri);
            reset_reponse_headers(&http_response);
          //  sleep(1);
            fclose(fpread);
        }
		
	}
    

    free(read_header);
	free(header_name);
	free(header_value);
	free(version);
	
	free(verb);
    return 0;
}

int check_dir(char *directoryname)
{
    DIR *dir;
    dir=opendir(directoryname);

    if(dir==NULL)
       return ERROR;
    else
    {
       closedir(dir);
       return 0;
    }
}

static void * thread_exec(void * arg)
{
  
    char readbuffer[MEMORYSIZE];
    int skfd=*(int *)arg;
    ssize_t bytes;
    int code;
    memset(readbuffer,0,sizeof(readbuffer));
    bytes=read(skfd,readbuffer,SIZE);  
    process_request(readbuffer,directory_name,skfd);
    free((int *)arg);
}

int main(int argc, char * argv[])
{

	int sockfd,ids;
    int newsockfd;
	struct sockaddr_in serv_addr;
	ssize_t bytes;
	char readbuf[SIZE];
	char *read_request=malloc(SIZE);
    int c;
    extern char *optarg;
    int check_fork=0,check_thread=0;
    long int port_num=0,default_port=9000;

    pthread_t tid;
    pthread_attr_t attr;
    
    int listenreturn;
    
    while((c=getopt(argc,argv,"ftp:d:"))!=-1)

    switch(c)
    {
        case 'p': if(optarg!=NULL)
                      port_num=atoi(optarg);
                  if(port_num==0)
                  {
                      printf("provide port number\n");
                      exit(EXIT_FAILURE);
                  }
                  break;
      
        case 'f': check_fork=1;
                  break;

        case 't': check_thread=1;
                  break;

        case 'd': if(optarg!=NULL)
                  {    
                      strcpy(directory_name,optarg);
                      int resultdir=check_dir(directory_name);
                      if(resultdir==ERROR)
                      {
                          printf("Directory does not exist\n");
                          exit(EXIT_FAILURE);
                      }
                  }
                  break;

        case '?': printf("Invalid Arguments");
                exit(EXIT_FAILURE);
    }

    if(check_fork==1 && check_thread==1)
    {
        printf("Cannot fork as well as thread\n");
        exit(EXIT_FAILURE);
    }
    if(port_num==0)
        port_num=default_port;
    

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
    	if(sockfd==ERROR)
	{
		perror("Socket Creation Error");
	}

    if(strcmp(directory_name,"")==0)
        strcpy(directory_name,".");
	memset(&serv_addr, '0', sizeof(serv_addr));
    	
    serv_addr.sin_family = AF_INET;
   	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port_num);
    
    int bindreturn=bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
	if(bindreturn==ERROR)
	{
		perror(NULL);
        exit(EXIT_FAILURE);
	}
    listenreturn=listen(sockfd, BACKLOG);

	pid_t childid;
    if(check_fork==0 && check_thread==0)
    {
        if(listenreturn==ERROR)
        {
            perror("Socket Listen Error");
        }

        while(1)
	    {
		    newsockfd=accept(sockfd,(struct sockaddr*)NULL,NULL);
            if(newsockfd==ERROR)
		    {
			    perror("Socket Accept Error");
		    }
             bytes=read(newsockfd,readbuf,SIZE);
             strncpy(read_request,readbuf,SIZE);
             process_request(read_request,directory_name,newsockfd);
             close(newsockfd);
        }
    }
    else if(check_fork==1)
    {
        if(listenreturn==ERROR)
        {
            perror("Socket Listen Error");
        }

        while(1)
        {
            newsockfd=accept(sockfd,(struct sockaddr*)NULL,NULL);

		    childid=fork();
		    if(childid==ERROR)
		    {
			    perror("Fork Error");
		    }
		    else if(childid==0)
		    {
			    bytes=read(newsockfd,readbuf,SIZE);
	            strncpy(read_request,readbuf,SIZE);
			    process_request(read_request,directory_name,newsockfd);
			    close(newsockfd);
			    free(read_request);
			    return(0);
		   }
           else
           {
               close(newsockfd);
               while(waitpid(ZOMBIECHILD,0,WNOHANG) > 0);
           }
        }
    }
    else if(check_thread==1)
    {
        while(1)
        {
            newsockfd=accept(sockfd,(struct sockaddr*)NULL,NULL);
            if(newsockfd==-1)
            {
                perror("Accept error");
            }
            int *ptr_sock=malloc(sizeof(int) * MEMORYSIZE);
            *ptr_sock=newsockfd; 
            int s = pthread_create(&tid,0,thread_exec,ptr_sock);

            if (s != 0) {
                perror("pthread create error");
                exit(EXIT_FAILURE);
            }
            pthread_detach(tid);
        }
    }
	free(read_request);
    close(sockfd);
	return(0);
}
