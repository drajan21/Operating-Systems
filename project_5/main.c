#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#define DIRNAME 200
#define ERROR -1
#define SIZE 16

int size;

volatile int active=1;

void handle_signal()
{
    active=0;
}

char getinput(int argc, char *argv[],char * directory_name, int *ptr_port, int  *thread_num, int *buf_size)
{
    int c,check_fork,check_thread,check_threadpool;;
    char strategy='s';
    extern char * optarg;
    int default_port=9000;

    while((c=getopt(argc,argv,"ftp:d:q:w:"))!=-1)
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

        case '?': printf("Invalid Arguments");
                exit(EXIT_FAILURE);
    }

    if((check_fork==1 && check_thread==1)||(check_thread==1 && check_threadpool==1)||(check_fork==1 && check_threadpool==1))
    {
        printf("Inappropriate options\n");
        exit(EXIT_FAILURE);
    }

    if(*buf_size==0)
        *buf_size=SIZE;

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

    char directory_name[DIRNAME];
    
    char strategy=getinput(argc,argv,directory_name,ptr_port,thread_num,buf_size);
    int sockfd=startserver(port_num);

/*    struct sigaction sa;
    sa.sa_flags=0;
    sa.sa_handler=handle_signal;
    if(sigaction(SIGINT,&sa,NULL)==-1)
        exit(EXIT_FAILURE);
*/
    if(strategy=='w')
    {
        printf("Calling thread pool with %d %d\n",*thread_num,*buf_size);
        dispatch_to_thread_pool(sockfd,directory_name,thread_num,buf_size);
    }
    else
    {
        while(1)
        {
            newsockfd=accept(sockfd,(struct sockaddr*)NULL,NULL);
            dispatch_connection(strategy,newsockfd,directory_name);
        }
    }
    
 //   printf("graceful exit\n");
    close(sockfd);
    return 0;
}
