/******************************************************************************************************************************************

FILE NAME: http_request.c
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
#include "main_functionality.h" 
#include "http_request.h"


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

double find_time_diff(struct timeval end, struct timeval start)
{
    long seconds = end.tv_sec - start.tv_sec;
    long micro_seconds = end.tv_usec - start.tv_usec;

    if(micro_seconds < 0)
    {
        seconds = seconds -1;
    } 
    double total_micro_seconds = (seconds * 1000000) + abs(micro_seconds);

    return total_micro_seconds;
}
int process_request(char *read_request,char *site,int newsockfd)
{

    FILE *fpread;
     
    struct timeval start_request_process;
    struct timeval finish_request_process;
    
    pthread_mutex_lock(&mtx_processtime); 
    gettimeofday(&start_request_process,NULL);
    pthread_mutex_unlock(&mtx_processtime);

    int majversion,minversion;
    http_method_t http_method;
    http_request_t http_request;
    http_response_t http_response;
    http_status_t http_status;
    char *verb = malloc(MEMORYSIZE);
    
    char path[MEMORYSIZE];
    memset(path,0,sizeof(path));

    char *version = malloc(MEMORYSIZE);
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
    
    pthread_mutex_lock(&mtx_processtime);
    gettimeofday(&finish_request_process,NULL);
    pthread_mutex_unlock(&mtx_processtime);

    pthread_mutex_lock(&mtx_processtime);
    //process_times[ind]=finish_request_process.tv_usec - start_request_process.tv_usec;
    process_times[ind] = find_time_diff(finish_request_process,start_request_process);
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

