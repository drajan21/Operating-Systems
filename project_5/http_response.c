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
#include <fcntl.h>
#include <unistd.h>
#include "http_response.h"

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
        printf("file not found\n");
       // writecheck=checkforerrorfiles(http_status,http_response);
       // strncpy(http_response->headers[POSITION_CONTENTTYPE].field_value,".html",MAX_HEADER_VALUE_LENGTH);
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
    printf("type:%s length:%s\n",http_response->headers[POSITION_CONTENTTYPE].field_value,http_response->headers[POSITION_CONTENTLENGTH].field_value);    
    free(uripath);
    return;
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
            perror("Stat Error");
            exit(EXIT_FAILURE);
            
		}  
    }
    fclose(fpread);
    return content_length;
}

int displayvals(http_response_t http_response)
{
      printf("HTTP/%d.%d %d %s\r\n",http_response.major_version,http_response.minor_version,http_response.status.code,http_response.status.reason);
      printf("%s: %s\r\n%s: %s\r\n%s: %s\r\n%s: %s\r\n\r\n",
              http_response.headers[POSITION_DATE].field_name,http_response.headers[POSITION_DATE].field_value,
                  http_response.headers[POSITION_SERVER].field_name,http_response.headers[POSITION_SERVER].field_value,
                      http_response.headers[POSITION_CONTENTTYPE].field_name,http_response.headers[POSITION_CONTENTTYPE].field_value,
                          http_response.headers[POSITION_CONTENTLENGTH].field_name,http_response.headers[POSITION_CONTENTLENGTH].field_value);

    return 0;
}

int sendresponse(int nsockfd,http_response_t http_response,char *fpath,char * site) 
{
    char responseheader[MEMORYSIZE];
    memset(responseheader,0,sizeof(char)* MEMORYSIZE);
 
    char responsestatus[MEMORYSIZE];
    memset(responsestatus,0,sizeof(char) * MEMORYSIZE);
    char *filepath=malloc(sizeof(char) * MEMORYSIZE);
    int file_fd;
    ssize_t numbytes;
    displayvals(http_response);
    sprintf(responsestatus,"HTTP/%d.%d %d %s\r\n",http_response.major_version,http_response.minor_version,http_response.status.code,http_response.status.reason);
    sprintf(responseheader,"%s: %s\r\n%s: %s\r\n%s: %s\r\n%s: %s\r\n\r\n",
    http_response.headers[POSITION_DATE].field_name,http_response.headers[POSITION_DATE].field_value,
    http_response.headers[POSITION_SERVER].field_name,http_response.headers[POSITION_SERVER].field_value,
    http_response.headers[POSITION_CONTENTTYPE].field_name,http_response.headers[POSITION_CONTENTTYPE].field_value,
    http_response.headers[POSITION_CONTENTLENGTH].field_name,http_response.headers[POSITION_CONTENTLENGTH].field_value);
     
    if(http_response.status.code!=HTTP_STATUS_LOOKUP[POSITION_OK].code)
    {
        filepath=getfilepath(http_response.status.code,site);
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
    free(filepath);
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
//    close(newfp);
    return 0;
}

char *getfilepath(int status,char * directory_name) 
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
