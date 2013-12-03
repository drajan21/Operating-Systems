#ifndef _STRATEGIES_H
#define _STRATEGIES_H

#include <signal.h>
#define MEMORYSIZE 1024
#define SIZE       100000
#define ERROR      -1 

ssize_t bytes;
char    readbuf[MEMORYSIZE];
char *  read_request;

static volatile sig_atomic_t active=1;
static volatile sig_atomic_t check=1;

int dispatch_to_serial(int fd, char * directory_name);
int dispatch_to_fork(int fd, char * directory_name);
int dispatch_to_thread(int fd, char * directory_name);
static void * thread_exec(void * arg);
int dispatch_to_thread_pool(int sockfd,char *name, int * thread_num,int *sz);
static void * thread_func();

#endif
