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
#include "strategies.h"
#include <signal.h>

#define ZOMBIECHILD -1

char dir_name[MEMORYSIZE];
int size;

//volatile sig_atomic_t active=1;
//volatile sig_atomic_t check=1;

/*void handle_sigint_t()
{
    activethread=0;
    activecheck=0;
}
*/
int dispatch_to_serial(int fd, char * directory_name)
{  
    bytes=read(fd,readbuf,MEMORYSIZE);
    process_request(readbuf,directory_name,fd);
    close(fd);
    return 0;
}

int dispatch_to_fork(int fd, char * directory_name)
{
    pid_t childid;
    childid=fork();
	if(childid==ERROR)
	{
	    perror("Fork Error"); 
	}
	else if(childid==0)
	{
	    bytes=read(fd,readbuf,SIZE);
        process_request(readbuf,directory_name,fd);
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
    strcpy(dir_name,name);
    pthread_t tid;
    int *ptr_sock=malloc(sizeof(int));
    *ptr_sock=fd;
    int s = pthread_create(&tid,0,&thread_exec,ptr_sock);
    if (s != 0) 
    {
        perror("pthread create error");
        exit(EXIT_FAILURE);
    }
    pthread_detach(tid);
    return 0;
}

static void * thread_exec(void * arg) 
{  
    int skfd=*(int *)arg;
    int code;
    bytes=read(skfd,readbuf,SIZE);
    process_request(readbuf,dir_name,skfd);
    close(skfd);
    free((int *)arg);
}

int dispatch_to_thread_pool(int sockfd,char *name, int * thread_num,int *sz)
{
    int newsockfd,counter,s;
    int no_of_threads=*thread_num;
    pthread_t tidarr[no_of_threads];

    size=*sz;
    memory_allocate(size);
    strcpy(dir_name,name);

    for(counter=0;counter<no_of_threads;counter++)
    {
        s=pthread_create(&tidarr[counter],0,&thread_func,NULL);
    }

    while(active)
    {
        check=1;
        newsockfd=accept(sockfd,(struct sockaddr *)NULL,NULL);
        if(check)
            thread_producer(name,newsockfd,size);
    }
    for(counter=0;counter<no_of_threads;counter++)
    {
        printf("Detaching\n");
        pthread_detach(tidarr[counter]);
    }
    return 0;
}

static void * thread_func()
{
    printf("size:%d\n",size);
    thread_consumer(size,dir_name);
}
