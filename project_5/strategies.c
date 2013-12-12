/******************************************************************************************************************************************

FILE NAME: strategies.c
STUDENT NAME:Diana Rajan
COURSE: CSCI 640
PROF. NAME: CHRIS MORRIS

******************************************************************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include "main_functionality.h"
#include "strategies.h"

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
    char readbuf[MEMORYSIZE];
    bytes=read(skfd,readbuf,MEMORYSIZE);
    process_request(readbuf,directory_name,skfd);
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
    pthread_t tid;
    int *ptr_sock=malloc(sizeof(int));
    *ptr_sock=fd;
    int s = pthread_create(&tid,0,&thread_exec,ptr_sock);
    if (s != 0) 
    {
        logt("ERROR","Thread Creation Error\n");
        exit(EXIT_FAILURE);
    }
    pthread_detach(tid);
    return 0;
}

