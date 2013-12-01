#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include "http_request.h"


int check_dir(char *directoryname)
{
    DIR *dir;
    dir=opendir(directoryname);

    if(dir==NULL)
        return -1;
    else
    {
        closedir(dir);
        return 0;
    }
}

int process_request(char *read_request,char *site,int newsockfd) 
{
 	
    http_method_t http_method;
    http_request_t http_request;
    http_response_t http_response;
    http_status_t http_status;

    memset(verb,0,sizeof(verb));
    memset(path,0,sizeof(path));
    memset(version,0,sizeof(version));

    sscanf(read_request,"%s %s %s",verb,path,version);
    int len=strlen(verb)+strlen(path)+strlen(version);
    sscanf(version,"HTTP/%d.%d",&http_request.major_version,&http_request.minor_version);
   // sscanf(version,"HTTP/%d.%d",&maj,&min);
    printf("%d\n",len);

    if(strcmp(path,"/favicon.ico")==0)
        return 0;
    
	http_method=parseverb(verb);
    process_path(path,site,&http_request);
//    parserequestheader(read_request,&http_request,len);
    http_request.method=http_method;
    
    int headercount=0,parseheaderval=0,headerline_length=0;
//    char *read_header;
//    read_header=(char *)malloc(MEMORYSIZE);
    char read_header[MEMORYSIZE];

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
    http_request.header_count=headercount;

    printf("method:%d path:%s count:%d\n",http_method,http_request.uri,http_request.header_count);
	
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
    sendresponse(newsockfd,http_response,http_request.uri,site);
    reset_reponse_headers(&http_response);

//    free(read_header);
//	free(version);
//	free(verb);
    return 0;
}

int check_file(char * filename)
{
    FILE *fpread;
    fpread=fopen(filename,"r");
     if(fpread)
        return 1;
    else
        return 0;

}

http_method_t parseverb(char *method)
{

    http_method_t http_method;
    http_method=INVALID_METHOD; 
    if(strcmp(method,"GET")==0)          return HTTP_METHOD_GET;
    else if(strcmp(method,"OPTIONS")==0) return HTTP_METHOD_OPTIONS;
    else if(strcmp(method,"HEAD")==0)    return HTTP_METHOD_HEAD;
    else if(strcmp(method,"POST")==0)    return HTTP_METHOD_POST;
    else if(strcmp(method,"PUT")==0)     return HTTP_METHOD_PUT;
    else if(strcmp(method,"DELETE")==0)  return HTTP_METHOD_DELETE;
    else if(strcmp(method,"TRACE")==0)   return HTTP_METHOD_TRACE;
    else if(strcmp(method,"CONNECT")==0) return HTTP_METHOD_CONNECT;
    else return INVALID_METHOD;
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

int parserequestheader(char * read_req,http_request_t * http_request,int len)
{
    int headercount=0,parseheaderval=0,headerline_length=0;
    char *read_header;
   // read_header=(char *)malloc(MEMORYSIZE);
    read_header[MEMORYSIZE];
    read_req=read_req+(len+4);
    /* Saving request headers */
    while(headercount!=MAX_HEADERS)
    {
       if(*(read_req+0)==13 && *(read_req+1)==10)
           break;
       sscanf(read_req,"%[^\n]",read_header);
       headerline_length=strlen(read_header);
       parseheaderval= preparerequestheader(read_header,headercount,http_request);
       headercount++;
       memset(read_header,'0',strlen(read_header));
       read_req=read_req+headerline_length+1;
    }
    http_request->header_count=headercount;
  //  free(read_header);
    return 0;
}

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
