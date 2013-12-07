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
#include <semaphore.h>
#include <signal.h>
#include <stdarg.h>
#include <sys/time.h>

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
#define BSIZE                      16


char directory_name[MEMORYSIZE];
int size,portno,threadno,buffersize,request_count,total_bytes;
//char *loglevel;
char strategy;
time_t server_start_time;
double process_times[500];
int ind,threadp_requests;

volatile sig_atomic_t active=1;

struct buff{
        int insert_position;
        int remove_position;
        int bounded_buff[];
       }*buff_ptr;

typedef struct buff buff_t;

struct log_level{
        int num;
        char name[10];
       };

typedef struct log_level log_level_t;
log_level_t level;

static pthread_mutex_t mtx_buff         = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cond_buff_full   = PTHREAD_COND_INITIALIZER;
static pthread_cond_t  cond_buff_empty  = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mtx_bytes        = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mtx_processtime  = PTHREAD_MUTEX_INITIALIZER;

void handle_signal()
{
    active=0;
}

void handle_sigusr1()
{
    send_stats();
}

int setloglevel(int levnum)
{
    switch(levnum)
    {
	case 0: strncpy(level.name,"ERROR",5);
                break;
        case 1: strncpy(level.name,"WARNING",7);
                break;
        case 2: strncpy(level.name,"INFO",4);
                break;
        case 3: strncpy(level.name,"DEBUG",5);
	        break;
    }
}

void handle_sigusr2()
{
    if(level.num==3)
        level.num=0;
    else
        level.num=level.num+1;
    setloglevel(level.num);
}
int logt(char * log_level, const char * message,...)
{
    va_list arg_list;
    int stream;
    FILE *fp;
    va_start(arg_list,message);
    switch(level.num)
    {
        case 0: if(strcmp(log_level,"ERROR")==0)
		    vfprintf(stderr,message,arg_list);
		if(strcmp(log_level,"DATA")==0)
		    vfprintf(stdout,message,arg_list);
                break;

        case 1: if(strcmp(log_level,"ERROR")==0)
		    vfprintf(stderr,message,arg_list);
		if(strcmp(log_level,"WARNING")==0)
		    vfprintf(stderr,message,arg_list);
		if(strcmp(log_level,"DATA")==0)
		    vfprintf(stdout,message,arg_list);
                break;

        case 2: if(strcmp(log_level,"ERROR")==0)
                    vfprintf(stderr,message,arg_list);
                if(strcmp(log_level,"WARNING")==0)
                    vfprintf(stderr,message,arg_list);
		if(strcmp(log_level,"INFO")==0)
		    vfprintf(stdout,message,arg_list);
                if(strcmp(log_level,"DATA")==0)
                    vfprintf(stdout,message,arg_list);
                break;

        case 3: if(strcmp(log_level,"ERROR")==0)
                    vfprintf(stderr,message,arg_list);
                if(strcmp(log_level,"WARNING")==0)
                    vfprintf(stderr,message,arg_list);
                if(strcmp(log_level,"INFO")==0)
                    vfprintf(stdout,message,arg_list);
		if(strcmp(log_level,"DEBUG")==0)
		    vfprintf(stdout,message,arg_list);
                if(strcmp(log_level,"DATA")==0)
                    vfprintf(stdout,message,arg_list);
                break;
    }
    va_end(arg_list);
    return 0;
}

/*
int logt(log_level_t log_level, const char * message,...)
{
    va_list arg_list;
    int stream;
    FILE *fp;
    switch(log_level.num)
    {
        case 0: fp=stderr;
                break;

        case 1: fp=stderr;
                break;

        case 2: fp=stdout;
                break;

        case 3: fp=stdout;
                break;
    }

    va_start(arg_list,message);
    vfprintf(fp,message,arg_list);
    va_end(arg_list);

    return 0;
}
*/
double get_total_processtime()
{
    int counter;
    double total_time;
    for(counter=0;counter<ind;counter++)
    {
        total_time = total_time + process_times[counter];
    }

    return total_time;
}

int send_stats()
{
   // log_level_t log_levelt;
   // char strat[10];
    time_t curr_time;
    double time_diff;
    curr_time=time(NULL);
    char *lev =malloc(5);
    strcpy(lev,"DATA");
    if(strategy == 'w')
        request_count = threadp_requests;
    
    time_diff=difftime(curr_time,server_start_time);
    double processtime_total=get_total_processtime();
    logt(lev,"Settings:---------------\n");
    logt(lev,"Document Root   : %s\n",directory_name);
    logt(lev,"Port No         : %d\n", portno);
    switch(strategy)
    {
        case 's': //strcpy(strat,"Serial\0");
		  logt(lev,"Strategy        : Serial\n");
                  break;
 
        case 'f': //strcpy(strat,"Fork\0");
                  logt(lev,"Strategy        : Fork\n");
                  break;

        case 't': //strcpy(strat,"Thread\0");
                  logt(lev,"Strategy        : Thread\n");
		  break;

        case 'w': //strcpy(strat,"Thread Pool\0");
                  logt(lev,"Strategy        : Thread Pool\n");
	          logt(lev,"Thread Pool Size: %d\n",threadno);
		  logt(lev,"BUffer Size     : %d\n",buffersize);
		  break;
    }
   /* logt(level,"Settings:---------------\n");
    logt(level,"Document Root : %s\n",directory_name);
    logt(level,"Port No       : %d\n", portno);
    
    logt(level,"Strategy      : %s\n",strat);
   // logt(level,"Strategy      : Thread Pool\n");
    */
    logt(lev,"Log Level       : %s\n",level.name);

    
    logt(lev,"\n\nStatistics-----------------------------------\n");
    logt(lev,"Total Uptime                      : %f secs\n",time_diff);
    logt(lev,"# of Requests handled             : %d\n",request_count);
    logt(lev,"Toal Processing Time              : %f secs\n",processtime_total/1000000);
    logt(lev,"Average Processing Time           : %f secs\n",(processtime_total/request_count)/1000000);
    logt(lev,"Total Amount of Data transfered   : %d bytes\n",total_bytes);
    
    free(lev);

    return 0;

}

int store_loglevel(char * loglevel)
{
   // log_level_t loglt;
    if(strcmp(loglevel,"ERROR")==0)   level.num = 0;
    if(strcmp(loglevel,"WARNING")==0) level.num = 1;
    if(strcmp(loglevel,"INFO")==0)    level.num = 2;
    if(strcmp(loglevel,"DEBUG")==0)   level.num = 3; 
    
    strcpy(level.name,loglevel); 
    printf("%s\n",level.name);
    return 0;

}



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

int sendcontent(char *path[], FILE *newfp)
{

    FILE *file; 
    int fd;
    size_t f_size;
    size_t wbytes;
    char *buffer = malloc(sizeof(char) * SIZE);
    memset(buffer,0, SIZE);
   // printf("File path: %s\n",path[0]);
    if(access(*path,F_OK)==0)
    {
        file = fopen(path[0], "r");
        if (file)
        {
            while((f_size= fread(buffer,1,SIZE,file)) > 0)
           // while((f_size=read(file,
	    wbytes=fwrite(buffer,1,f_size,newfp);
        }
        else
        {
            logt("ERROR","Error Opening File while sending content\n");
           // perror("Error Opening File");
        }
        fclose(file);
    }
    else
        logt("ERROR","Error accessing path while sending content\n");
        //perror(NULL);
   
    free(buffer);
    //fclose(newfp);
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
	    {
                strcpy(pathname,path400);
		fclose(fp);
	    }
        }
        else
	{
            strcpy(pathname,path404);
	    fclose(fp);
	}
    }

    free(path404);
    free(path400);

    return pathname;

}

int sendresponse(int nsockfd,http_response_t http_response,char *fpath)
{
    FILE *fnsockfd=fdopen(nsockfd,"w");
        char responseheader[MEMORYSIZE];
    memset(responseheader,0,sizeof(char)* MEMORYSIZE);

    char responsestatus[MEMORYSIZE];
        memset(responsestatus,0,sizeof(char) * MEMORYSIZE);

    char *filepath=malloc(sizeof(char) * MEMORYSIZE);
    memset(filepath,0,sizeof(char) * MEMORYSIZE);

    int file_fd;
        ssize_t numbytes;

    fprintf(fnsockfd,"HTTP/%d.%d %d %s\r\n",http_response.major_version,http_response.minor_version,http_response.status.code,http_response.status.reason);
    fprintf(fnsockfd,"%s: %s\r\n%s: %s\r\n%s: %s\r\n%s: %s\r\n\r\n",
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

   /* numbytes=write(nsockfd,responsestatus,strlen(responsestatus));
        if(numbytes==ERROR)
        {
                logt(level,"Write to Socket Error while sending response\n");
        //perror("Write to Socket Error");
        }
        numbytes=write(nsockfd,responseheader,strlen(responseheader));
        if(numbytes==ERROR)
    {
       logt(level,"Write to Socket Error while sending response\n");
       // perror("Write to Socket Error");
    }*/

    sendcontent(&filepath,fnsockfd);
    fclose(fnsockfd);
    free(filepath);
    return 0;
}



/*
int sendresponse(int nsockfd,http_response_t http_response,char *fpath)
{
	char responseheader[MEMORYSIZE];
    memset(responseheader,0,sizeof(char)* MEMORYSIZE);
 
    char responsestatus[MEMORYSIZE];
	memset(responsestatus,0,sizeof(char) * MEMORYSIZE);

    char *filepath=malloc(sizeof(char) * MEMORYSIZE);
    memset(filepath,0,sizeof(char) * MEMORYSIZE);

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
		logt(level,"Write to Socket Error while sending response\n");
        //perror("Write to Socket Error");
	}
	numbytes=write(nsockfd,responseheader,strlen(responseheader));
	if(numbytes==ERROR)
    {
       logt(level,"Write to Socket Error while sending response\n");
       // perror("Write to Socket Error");
    }

    sendcontent(&filepath,nsockfd);
//    close(nsockfd);
    free(filepath);
    return 0;
}
*/
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
	     fclose(fpread);  
	 }
	 return content_length;
}

int checkforerrorfiles(http_status_t *status,http_response_t *http_response)
{
    FILE *fpcheck;
    int file_fd,writecheck;
    //ssize_t numread;
    int numread;
    int checkfile=1,length;
    char *filename404=malloc(sizeof(char) * MEMORYSIZE);
    char *filename400=malloc(sizeof(char) * MEMORYSIZE);

    char readbuf[MEMORYSIZE],writebuf[MEMORYSIZE],lengthbuffer[MEMORYSIZE];
    strcpy(filename404,directory_name);
    strcat(filename404,"/404.html");

    strcpy(filename400,directory_name);
    strcat(filename400,"/400.html");
//    printf("%s\n",filename404);
//    printf("%s\n",filename400);
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
                    {
                        length=find_content_length(filename400,status);
                        fclose(fpcheck);
                    }
                }
                else
                {
                    length=find_content_length(filename404,status);
                    fclose(fpcheck);
                }
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
                    logt("ERROR","File Open Error\n");    
                    //perror("File Open Error");
                else
                {
                    numread=read(file_fd,readbuf,MEMORYSIZE);
                    if(numread==0)
                    {
                        sprintf(writebuf,"404 Error-Page Not Found");
                        writecheck=write(file_fd,writebuf,strlen(writebuf));
                        if(writecheck==-1)
                            logt("ERROR","File Write Error\n");
                            //perror("File Write Error");
                        else
                        {
                            //sprintf(lengthbuffer,"%d",strlen(writebuf));
			    sprintf(lengthbuffer,"%d",24);
			    //strcpy(lengthbuffer,strlen(writebuf))
                        }
                    }
                    else
                        sprintf(lengthbuffer,"%d",numread);
                    close(file_fd);
                 }
        }
        strncpy(http_response->headers[POSITION_CONTENTLENGTH].field_value,lengthbuffer,MAX_HEADER_VALUE_LENGTH);
    }
    free(filename404);
    free(filename400);

    return checkfile;
}
 
/* Function to Build a response */
int buildresponse(http_request_t *http_request,http_status_t *http_status,http_response_t *http_response)
{
	int file_fd,length;
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
        strncpy(http_response->headers[POSITION_CONTENTTYPE].field_value,contenttype,MAX_HEADER_VALUE_LENGTH);
		strncpy(http_response->headers[POSITION_CONTENTLENGTH].field_name,"Content-Length",MAX_HEADER_NAME_LENGTH);
		length=find_content_length(http_request->uri,http_status);
		sprintf(lengthbuffer,"%d",length);
		strncpy(http_response->headers[POSITION_CONTENTLENGTH].field_value,lengthbuffer,MAX_HEADER_VALUE_LENGTH);
	}
    headerindex++;
	http_response->header_count=headerindex;
    
    pthread_mutex_lock(&mtx_bytes);
    total_bytes=total_bytes + length;
    pthread_mutex_unlock(&mtx_bytes);
	
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

	if(strcmp(method,"GET")==0)     http_method= HTTP_METHOD_GET;
	if(strcmp(method,"OPTIONS")==0) http_method= HTTP_METHOD_OPTIONS;
	if(strcmp(method,"HEAD")==0)    http_method= HTTP_METHOD_HEAD;
    if(strcmp(method,"POST")==0)    http_method= HTTP_METHOD_POST;
    if(strcmp(method,"PUT")==0)     http_method= HTTP_METHOD_PUT;
    if(strcmp(method,"DELETE")==0)  http_method= HTTP_METHOD_DELETE;
	if(strcmp(method,"TRACE")==0)   http_method= HTTP_METHOD_TRACE;
    if(strcmp(method,"CONNECT")==0) http_method= HTTP_METHOD_CONNECT;

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

int check_file(char * filename)
{
    FILE *fpread;
    int returnval;
    fpread=fopen(filename,"r");
    if(fpread)
    {
        returnval = 1;
        fclose(fpread);
    } 
    else
    {
	returnval = 0;
    }
    return returnval;
}

int process_request(char *read_request,char *site,int newsockfd)
{

     FILE *fpread;
     
     struct timeval start_request_process;
     struct timeval finish_request_process;
     
    // request_count++;

     gettimeofday(&start_request_process,NULL);
    // printf(ctime(&start_request_process));

	 int majversion,minversion;
     http_method_t http_method;
     http_request_t http_request;
     http_response_t http_response;
     http_status_t http_status;

	char *verb;
    verb=malloc(MEMORYSIZE);

    char path[MEMORYSIZE];
    memset(path,0,sizeof(path));

    char *version;
    version=malloc(MEMORYSIZE);

	char *header_name,*header_value;
	header_name=malloc(MEMORYSIZE);
	header_value=malloc(MEMORYSIZE);
	
	sscanf(read_request,"%s %s %s",verb,path,version);
	sscanf(version,"HTTP/%d.%d",&http_request.major_version,&http_request.minor_version);	
    
  /*  if(strcmp(path,"/favicon.ico")==0)
        return 0;
    */
	http_method=parseverb(verb);

    process_path(path,site,&http_request);


	int headercount=0,parseheaderval=0,headerline_length=0;
	char *read_header;
	read_header=malloc(MEMORYSIZE);

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
              
        int filecheck=check_file(http_request.uri);
		if(filecheck==0)
        {
            http_status.code=HTTP_STATUS_LOOKUP[POSITION_NOTFOUND].code;
            http_status.reason=HTTP_STATUS_LOOKUP[POSITION_NOTFOUND].reason;
	       
        }
        else
        {
		    http_status.code=HTTP_STATUS_LOOKUP[POSITION_OK].code;
            http_status.reason=HTTP_STATUS_LOOKUP[POSITION_OK].reason;
		   
        }
		
	}
    
    buildresponse(&http_request,&http_status,&http_response);
	sendresponse(newsockfd,http_response,http_request.uri);
    
    gettimeofday(&finish_request_process,NULL);
   // printf(ctime(&finish_request_process));

    pthread_mutex_lock(&mtx_processtime);
    process_times[ind]=finish_request_process.tv_usec - start_request_process.tv_usec;
    ind++;
    pthread_mutex_unlock(&mtx_processtime);
    

    reset_reponse_headers(&http_response);
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

/*static void * thread_exec(void * arg)
{
  
    char readbuffer[MEMORYSIZE];
    int skfd=*(int *)arg;
    ssize_t bytes;
    int code;
    memset(readbuffer,0,sizeof(readbuffer));
    bytes=read(skfd,readbuffer,SIZE);  
    process_request(readbuffer,directory_name,skfd);
    free((int *)arg);
}*/

int startserver(int port)
{
    int sockfd;
    struct sockaddr_in serv_addr;

    memset(&serv_addr, '0', sizeof(serv_addr));
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd==ERROR)
    {
        logt("ERROR","Socket Creation Error\n");
        //perror("Socket Creation Error");
        exit(EXIT_FAILURE);    
    }
    int option=1;
    if(setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&option,sizeof(option))<0)
    {
	logt("ERROR","Reuse Address Failed\n");
	exit(EXIT_FAILURE);
    }    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);
                            
    int bindreturn=bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if(bindreturn==ERROR)
    {
        logt("ERROR","Bind Error\n");
        //perror(NULL);
        exit(EXIT_FAILURE); 
    }
    int listenreturn=listen(sockfd, BACKLOG);
    if(listenreturn==ERROR)
    {
        logt("ERROR","Listen Error\n");
        //perror("Listen Error:");
        exit(EXIT_FAILURE);
    }
    return sockfd;
}



char getinput(int argc, char *argv[],char * directory_name)
{
    int c,check_fork,check_thread,check_threadpool;;
    char strategy='s';
    extern char * optarg;
    int default_port=9000;
    char * loglevel = malloc(BSIZE);
    memset(loglevel,0,BSIZE);

    while((c=getopt(argc,argv,"ftp:d:q:w:v:"))!=-1)
    switch(c) 
    { 
        case 'p': if(optarg!=NULL)
                      portno=atoi(optarg);
                 /* if(*ptr_port==0)
                  {
                      printf("provide port number\n");
                      exit(EXIT_FAILURE);
                  }*/
                  break;
      
        case 'f': check_fork=1;
                  strategy='f';
                  break;
        case 't': check_thread=1;
                  strategy='t';
                  break;
        case 'd': if(optarg!=NULL)
                  {
                      strcpy(directory_name,optarg);
                      int resultdir=check_dir(directory_name);
                      if(resultdir==ERROR)
                      {
                          logt("DEBUG","Directory Does Not Exist\n");
                          //printf("Directory does not exist\n");
                          exit(EXIT_FAILURE);
                      }
                  }
                  break;
        case 'w': if(optarg!=NULL)
                    threadno=atoi(optarg);
                  strategy='w';
                  check_threadpool=1;
                  break;

        case 'q': if(optarg!=NULL)
                    buffersize=atoi(optarg);
                  break;

        case 'v': if(optarg!=NULL)
                    strcpy(loglevel,optarg);
                  break;

        case '?': printf("Invalid Arguments");
                exit(EXIT_FAILURE);
    }

    if((check_fork==1 && check_thread==1)||(check_thread==1 && check_threadpool==1)||(check_fork==1 && check_threadpool==1))
    {
        logt("WARNING","Inappropriate Options\n");
        //printf("Inappropriate options\n");
        exit(EXIT_FAILURE);
    }
    
    
    printf("log:%s\n",loglevel);
    if(strcmp(loglevel,"")==0)
        strcpy(loglevel,"WARNING");
      store_loglevel(loglevel);

    if(buffersize==0)
        buffersize=BSIZE;

    if(portno==0)
        portno=default_port;

    free(loglevel);
    return strategy;
}

static void * thread_exec(void * arg) 
{ 
    sigset_t sig_new,sig_old;
    sigemptyset(&sig_new);
    sigaddset(&sig_new,SIGINT);
    sigaddset(&sig_new,SIGTERM);
    sigaddset(&sig_new,SIGUSR1);
    sigaddset(&sig_new,SIGUSR2);
    pthread_sigmask(SIG_BLOCK,&sig_new,&sig_old);
 
    size_t bytes;
    int skfd=*(int *)arg;
    int code;
   // char *readbuf=malloc(MEMORYSIZE);
    char readbuf[MEMORYSIZE];
    bytes=read(skfd,readbuf,MEMORYSIZE);
    process_request(readbuf,directory_name,skfd);
   // close(skfd);
   // free(readbuf);
    free((int *)arg);
}


int dispatch_to_serial(char * read_request,int fd, char * directory_name)
{  
    size_t bytes;
    bytes=read(fd,read_request,MEMORYSIZE);
    process_request(read_request,directory_name,fd);
    close(fd);
    return 0;
}

int dispatch_to_fork(char * read_request,int fd, char * directory_name)
{
    size_t bytes;
    pid_t childid;
    childid=fork();
	if(childid==ERROR)
	{
	    logt("ERROR","Fork Error\n");
        //perror("Fork Error"); 
	}
	else if(childid==0)
	{
	    bytes=read(fd,read_request,MEMORYSIZE);
        process_request(read_request,directory_name,fd);
		close(fd);
		return 0;
	}
    else
    {
        close(fd);
        while(waitpid(ZOMBIECHILD,0,WNOHANG) > 0);
    }
    return 0;
}

int dispatch_to_thread(int fd, char * name)
{
    size_t bytes;
   // strcpy(dir_name,name);
    pthread_t tid;
    int *ptr_sock=malloc(sizeof(int));
    *ptr_sock=fd;
    int s = pthread_create(&tid,0,&thread_exec,ptr_sock);
    if (s != 0) 
    {
        logt("ERROR","Thread Creation Error\n");
        //perror("pthread create error");
        exit(EXIT_FAILURE);
    }
    pthread_detach(tid);
    return 0;
}



int dispatch_connection(char * read_request,char strategy,int fd,char * directory_name)
{
    switch(strategy)
    {
        case 's': dispatch_to_serial(read_request,fd,directory_name);
                  break;

        case 'f': dispatch_to_fork(read_request,fd,directory_name);
                  break;

        case 't': dispatch_to_thread(fd,directory_name);
                  break;
    }

    return 0;
}




int memory_allocate(int size)
{
     buff_ptr=(buff_t *)malloc(sizeof(buff_t)* size );
     buff_ptr->insert_position=0;
     buff_ptr->remove_position=0;

    return 0;
}

int thread_producer(char *name,int fd,int size)
{
    pthread_mutex_lock(&mtx_buff);
    int curr_position = (buff_ptr->insert_position + 1) % size;
    while(buff_ptr->remove_position==curr_position)
    {
        pthread_cond_wait(&cond_buff_full,&mtx_buff);
    }
    buff_ptr->bounded_buff[buff_ptr->insert_position]=fd;
    pthread_cond_broadcast(&cond_buff_empty);
    buff_ptr->insert_position=curr_position;
    pthread_mutex_unlock(&mtx_buff);

    return 0;
}

int thread_consumer(char * readbuf,int size,char * dir_name)
{
    size_t bytes;
    while(active)
    {
        pthread_mutex_lock(&mtx_buff);
        while(buff_ptr->remove_position == buff_ptr->insert_position)
        {
            pthread_cond_wait(&cond_buff_empty,&mtx_buff);
        }
        int curr_position = (buff_ptr->remove_position+1) % size;
        int fd = buff_ptr->bounded_buff[buff_ptr->remove_position]; 
        pthread_cond_broadcast(&cond_buff_full);
        buff_ptr->remove_position = curr_position;
        pthread_mutex_unlock(&mtx_buff);
       // if(check)
       // {
            bytes=read(fd,readbuf,MEMORYSIZE);
            process_request(readbuf,dir_name,fd);
       // }
       // close(fd);
    }
    return 0;
}


static void * thread_func()
{
    //char * readbuf=malloc(MEMORYSIZE);
    char readbuf[MEMORYSIZE];
    thread_consumer(readbuf,buffersize,directory_name);
}

int dispatch_to_thread_pool(int sockfd,char *name)
{
    int newsockfd,counter,s;
    int no_of_threads=threadno;
    pthread_t tidarr[no_of_threads];

//    printf("%d\n",buffersize);
    memory_allocate(buffersize);
    
    sigset_t signew,sigold;
    sigemptyset(&signew);
    sigaddset(&signew,SIGINT);
    sigaddset(&signew,SIGTERM);
    sigaddset(&signew,SIGUSR1);
    sigaddset(&signew,SIGUSR2);
    pthread_sigmask(SIG_BLOCK,&signew,&sigold);

    for(counter=0;counter<no_of_threads;counter++)
    {
        s=pthread_create(&tidarr[counter],0,&thread_func,NULL);
    }
    
    pthread_sigmask(SIG_SETMASK,&sigold,NULL);
    while(active)
    {
        //check=1;
        newsockfd=accept(sockfd,(struct sockaddr *)NULL,NULL);
        if(newsockfd<0)
        {
            continue;
        }
        threadp_requests++;
        if(newsockfd>0)
            thread_producer(directory_name,newsockfd,buffersize);
    }
    close(newsockfd);
    for(counter=0;counter<no_of_threads;counter++)
    {
        logt("DEBUG","Detaching thread\n");
        pthread_detach(tidarr[counter]);
    }
    free(buff_ptr);
    return 0;
}


int main(int argc, char * argv[])
{

	int sockfd;
    int newsockfd;
//	char readbuf[SIZE];
	char *read_request=malloc(MEMORYSIZE);
    long int port_num=0,default_port=9000;

    
    struct sigaction sa,sa_usr1,sa_usr2;
    sa.sa_flags=0;
    sa.sa_handler=handle_signal;

    if(sigaction(SIGINT,&sa,NULL)==-1)
        exit(EXIT_FAILURE);
    if(sigaction(SIGTERM,&sa,NULL)==-1)
        exit(EXIT_FAILURE);

    sa_usr1.sa_flags=0;
    sa_usr1.sa_handler=handle_sigusr1;
    if(sigaction(SIGUSR1,&sa_usr1,NULL)==-1)
        exit(EXIT_FAILURE);
    
    sa_usr2.sa_flags=0;
    sa_usr2.sa_handler=handle_sigusr2;
    if(sigaction(SIGUSR2,&sa_usr2,NULL)==-1)
        exit(EXIT_FAILURE);

    //logt("INFO","Process Id:%d\n",getpid());
    strategy=getinput(argc,argv,directory_name); 
    logt("INFO","Proess Id:%d\n",getpid());

    sockfd=startserver(portno);
    time(&server_start_time);
    //printf(ctime(&server_start_time));
    if(strategy=='w')
    {
        //printf("Calling thread pool with %d %d\n",*thread_num,*buf_size);
//        active=0;
        dispatch_to_thread_pool(sockfd,directory_name);
    }
    else
    {
        while(active)
        {
//        check=1;
            newsockfd=accept(sockfd,(struct sockaddr*)NULL,NULL);
            if(newsockfd<0)
            {
                continue;
            }
            request_count++;
            if(newsockfd>0)
                dispatch_connection(read_request,strategy,newsockfd,directory_name);
        }
     }
	
	free(read_request);
    close(newsockfd);
    close(sockfd);
    logt("DEBUG","Graceful Exit\n");
    return(0);
}
