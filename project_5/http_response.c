/******************************************************************************************************************************************

FILE NAME: http_response.c
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
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>>
#include <sys/time.h>
#include "http_response.h"
#include "main_functionality.h"

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

    if(access(*path,F_OK)==0)
    {
        file = fopen(path[0], "r");
        if (file)
        {
            while((f_size= fread(buffer,1,SIZE,file)) > 0)
	    wbytes=fwrite(buffer,1,f_size,newfp);
        }
        else
        {
            logt("ERROR","Error Opening File while sending content\n");
        }
        fclose(file);
    }
    else
        logt("ERROR","Error accessing path while sending content\n");
   
    free(buffer);
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

    sendcontent(&filepath,fnsockfd);
    fclose(fnsockfd);
    free(filepath);
    return 0;
}

int checkforerrorfiles(http_status_t *status,http_response_t *http_response)
{
    FILE *fpcheck;
    int file_fd,writecheck;
    int numread;
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
                else
                {
                    numread=read(file_fd,readbuf,MEMORYSIZE);
                    if(numread==0)
                    {
                        sprintf(writebuf,"404 Error-Page Not Found");
                        writecheck=write(file_fd,writebuf,strlen(writebuf));
                        if(writecheck==-1)
                            logt("ERROR","File Write Error\n");
                        else
                        {
			    sprintf(lengthbuffer,"%d",24);
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

    char content_type[50];
    memset(content_type,0,sizeof(content_type));
    if(uripath)
        parsepath(uripath,content_type);
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
        strncpy(http_response->headers[POSITION_CONTENTTYPE].field_value,content_type,MAX_HEADER_VALUE_LENGTH);
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
