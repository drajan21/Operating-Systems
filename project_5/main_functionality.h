/******************************************************************************************************************************************

FILE NAME: main_functionality.h
STUDENT NAME:Diana Rajan
COURSE: CSCI 640
PROF. NAME: CHRIS MORRIS

******************************************************************************************************************************************/
#ifndef _MAIN_H
#define _MAIN_H

#include <time.h>
#include <signal.h>
#include <pthread.h>
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


char directory_name[MEMORYSIZE];
int size,portno,threadno,buffersize,request_count,total_bytes;
char strategy;
time_t server_start_time;
double process_times[500];
//memset(process_times,0,sizeof(process_times));
int ind,threadp_requests;

extern volatile sig_atomic_t active;

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
static pthread_mutex_t mtx_starttime    = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mtx_endtime      = PTHREAD_MUTEX_INITIALIZER;

void handle_signal();
void handle_sigusr1();
void handle_sigusr2();
double get_total_processtime();
int send_stats();
int store_loglevel(char * loglevel);
int find_content_length(char *filepath, http_status_t *http_status);
char getinput(int argc, char *argv[],char * directory_name);
int dispatch_connection(char * read_request,char strategy,int fd,char * directory_name);
static void * thread_func();
int dispatch_to_thread_pool(int sockfd,char *name);

#endif
