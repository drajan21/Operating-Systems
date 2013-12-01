#ifndef _HTTP_REQUEST_H
#define _HTTP_REQUEST_H

#include "csuc_http.h"

#define MEMORYSIZE                 1024
#define INVALID_METHOD             -1
#define POSITION_OK                2
#define BACKLOG                    10
#define POSITION_BADREQUEST        16
#define POSITION_NOTFOUND          20
#define POSITION_INTERNALERROR     34
#define POSITION_NOTIMPLEMENTED    35

char verb[MEMORYSIZE],version[MEMORYSIZE];
char path[MEMORYSIZE];


int check_dir(char * directoryname);
int check_file(char * filename);
http_method_t parseverb(char *method);
int process_path(char *path, char *site,http_request_t *http_request);
int process_request(char *read_request,char *site,int newsockfd);
int parserequestheader(char * read_request,http_request_t * http_request,int len);
int preparerequestheader(const char *headerline,int headerindex,http_request_t *http_request);
int reset_reponse_headers(http_response_t *http_response);



const http_status_t HTTP_STATUS_LOOKUP[] = {
    // Informational Status Codes
    // Request received, continuing process#endif
    {100, "Continue"},
    {101, "Switching Protocols"},
    // Success Status Codes
    // The action was successfully received, understood, and accepted
    {200, "OK"},
    {201, "Created"},
    {202, "Accepted"},
    {203, "Non-Authoritative Information"},
    {204, "No Content"},
    {205, "Reset Content"},
    {206, "Partial Content"},
    // Redirection Status Codes
    // Further action must be taken in order to complete the request
    {300, "Multiple Choices"},
    {301, "Moved Permanently"},
    {302, "Found"},
    {303, "See Other"},
    {304, "Not Modified"},
    {305, "Use Proxy"},
    {307, "Temporary Redirect"},
    // Client Error Status Codes
    // The request contains bad syntax or cannot be fulfilled
    {400, "Bad Request"},
    {401, "Unauthorized"},
    {402, "Payment Required"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {405, "Method Not Allowed"},
    {406, "Not Acceptable"},
    {407, "Proxy Authentication Required"},
    {408, "Request Time-out"},
    {409, "Conflict"},
    {410, "Gone"},
    {411, "Length Required"},
    {412, "Precondition Failed"},
    {413, "Request Entity Too Large"},
    {414, "Request-URI Too Large"},
    {415, "Unsupported Media Type"},
    {416, "Requested range not satisfiable"},
    {417, "Expectation Failed"},
    // Server Error Status Codes
    // The server failed to fulfill an apparently valid request
    {500, "Internal Server Error"},
    {501, "Not Implemented"},
    {502, "Bad Gateway"},
    {503, "Service Unavailable"},
    {504, "Gateway Time-out"},
    {505, "HTTP Version not supported"}
};

#endif
