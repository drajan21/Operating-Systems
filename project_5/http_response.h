#ifndef _HTTP_RESPONSE_H
#define _HTTP_RESPONSE_H

#include "csuc_http.h"

#define MEMORYSIZE                 1000
#define POSITION_OK                2
#define POSITION_BADREQUEST        16
#define POSITION_NOTFOUND          20
#define POSITION_INTERNALERROR     34
#define POSITION_NOTIMPLEMENTED    35
#define POSITION_SERVER            3
#define POSITION_DATE              2
#define POSITION_CONTENTTYPE       4
#define POSITION_CONTENTLENGTH     5
#define SIZE                       1000000
#define ERROR                      -1

int buildresponse(http_request_t *http_request,http_status_t *http_status,http_response_t *http_response);
int getdatetime(http_response_t *http_response);
char * parsepath(char *path);
int find_content_length(char *filepath, http_status_t *http_status);
int sendcontent(char *path[], int newfp);
int sendresponse(int nsockfd,http_response_t http_response,char *fp, char * site);
char *getfilepath(int status, char * directory_name);

#endif
