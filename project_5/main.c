#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "strategies.h"

#define DIRNAME 200
#define ERROR -1
#define BSIZE 16

int size,portno,threadno,buffersize;
char *loglevel;
char strategy;
char directory_name[DIRNAME];

//sig_atomic_t active=1;
//sig_atomic_t check=1;

//active=1;
//check=1;

struct log_level{
        int num;
        char name[];
       };

struct log_level log_level_t;

void handle_signal()
{
    active=0;
    check=0;
    printf("Signal cought\n");
}

void handle_sigusr1()
{
    check=0;
    printf("signal raised\n");
    send_stats();
}

int set_variables(int *ptr_port, int * thread_num, int * buf_size)
{
    portno=*ptr_port;
    threadno=*thread_num;
    buffersize=*buf_size;

    return 0;
}


int send_stats()
{
    char strat[10];
    switch(strategy)
    {
        case 's': strcpy(strat,"Serial");
                  break;
 
        case 'f': strcpy(strat,"Fork");
                  break;

        case 't': strcpy(strat,"Thread");
                  break;

        case 'w': strcpy(strat,"Thread Pool");
                  break;
    }
    logt(log_level_t.num,"Settings:---------------\n");
    logt(log_level_t.num,"Document Root : %s\n",directory_name);
    logt(log_level_t.num,"Port No : %d\n", portno);
    logt(log_level_t.num,"Strategy : %s\n",strat);
    logt(log_level_t.num,"Log Level : %s\n",log_level_t.name);

    return 0;

}

int store_loglevel(char * loglevel)
{
    if(strcmp(loglevel,"ERROR")==0)   log_level_t.num = 0;
    if(strcmp(loglevel,"WARNING")==0) log_level_t.num = 1;
    if(strcmp(loglevel,"INFO")==0)    log_level_t.num = 2;
    if(strcmp(loglevel,"DEBUG")==0)   log_level_t.num = 3; 
    
    strcpy(log_level_t.name,loglevel);
 
    return 0;
}

char getinput(int argc, char *argv[],char * directory_name, int *ptr_port, int  *thread_num, int *buf_size)
{
    int c,check_fork,check_thread,check_threadpool;;
    char strategy='s';
    extern char * optarg;
    int default_port=9000;
    loglevel = malloc(BSIZE);

    while((c=getopt(argc,argv,"ftp:d:q:w:v:"))!=-1)
    switch(c) 
    { 
        case 'p': if(optarg!=NULL)
                      *ptr_port=atoi(optarg);
                  if(*ptr_port==0)
                  {
                      printf("provide port number\n");
                      exit(EXIT_FAILURE);
                  }
                  break;
      
        case 'f': check_fork=1;
                  strategy='f';
                  break;
        case 't': check_thread=1;
                  strategy='t';
                  break;
        case 'd': if(optarg!=NULL)
                  {
                      strcpy(directory_name,optarg);
                      int resultdir=check_dir(directory_name);
                      if(resultdir==ERROR)
                      {
                          printf("Directory does not exist\n");
                          exit(EXIT_FAILURE);
                      }
                  }
                  break;
        case 'w': if(optarg!=NULL)
                    *thread_num=atoi(optarg);
                  strategy='w';
                  check_threadpool=1;
                  break;

        case 'q': if(optarg!=NULL)
                    *buf_size=atoi(optarg);
                  break;

        case 'v': if(optarg!=NULL)
                    strcpy(loglevel,optarg);
                  break;

        case '?': printf("Invalid Arguments");
                exit(EXIT_FAILURE);
    }

    if((check_fork==1 && check_thread==1)||(check_thread==1 && check_threadpool==1)||(check_fork==1 && check_threadpool==1))
    {
        printf("Inappropriate options\n");
        exit(EXIT_FAILURE);
    }
 
    if(strcmp(loglevel,"")==0)
        strcpy(loglevel,"WARNING");
    store_loglevel(loglevel);

    if(*buf_size==0)
        *buf_size=BSIZE;

    if(*ptr_port==0)
        *ptr_port=default_port;

    return strategy;
}

int dispatch_connection(char strategy,int fd,char * directory_name)
{
    switch(strategy)
    {
        case 's': dispatch_to_serial(fd,directory_name);
                  break;

        case 'f': dispatch_to_fork(fd,directory_name);
                  break;

        case 't': dispatch_to_thread(fd,directory_name);
                  break;
    }

    return 0;
}

int main(int argc, char * argv[])
{
    int port_num=0,newsockfd,threads=0,buf_sz=0;
    int *ptr_port,*thread_num;
    int *buf_size;
    ptr_port=&port_num;
    thread_num=&threads;
    buf_size=&buf_sz;

    printf("%d\n",getpid());

    strategy=getinput(argc,argv,directory_name,ptr_port,thread_num,buf_size);  
    set_variables(ptr_port,thread_num,buf_size);

    int sockfd=startserver(port_num);

    struct sigaction sa,sa_usr1;
    sa.sa_flags=0;
    sa.sa_handler=handle_signal;

    if(sigaction(SIGINT,&sa,NULL)==-1)
        exit(EXIT_FAILURE);
    if(sigaction(SIGTERM,&sa,NULL)==-1)
        exit(EXIT_FAILURE);

    sa_usr1.sa_flags=0;
    sa_usr1.sa_handler=handle_sigusr1;
    if(sigaction(SIGUSR1,&sa_usr1,NULL)==-1)
        exit(EXIT_FAILURE);

    if(strategy=='w')
    {
        //printf("Calling thread pool with %d %d\n",*thread_num,*buf_size);
        active=0;
        dispatch_to_thread_pool(sockfd,directory_name,thread_num,buf_size);
    }
    while(active)
    {
        check=1;
        newsockfd=accept(sockfd,(struct sockaddr*)NULL,NULL);
        if(check)
            dispatch_connection(strategy,newsockfd,directory_name);
    }
    
    
    printf("graceful exit\n");
    close(sockfd);
    return 0;
}
