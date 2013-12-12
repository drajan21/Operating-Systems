/******************************************************************************************************************************************

FILE NAME: http_response.h
STUDENT NAME:Diana Rajan
COURSE: CSCI 640
PROF. NAME: CHRIS MORRIS

******************************************************************************************************************************************/

#ifndef _HTTP_RESPONSE_H
#define _HTTP_RESPONSE_H

#include "csuc_http.h"

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

int getdatetime(http_response_t *http_response);
int sendcontent(char *path[], FILE *newfp);
char *getfilepath(int status);
int sendresponse(int nsockfd,http_response_t http_response,char *fpath);
int checkforerrorfiles(http_status_t *status,http_response_t *http_response);
int buildresponse(http_request_t *http_request,http_status_t *http_status,http_response_t *http_response);
#endif;
