/******************************************************************************************************************************************

FILE NAME: main_functionality.c
STUDENT NAME:Diana Rajan
COURSE: CSCI 640
PROF. NAME: CHRIS MORRIS

******************************************************************************************************************************************/
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
#include <semaphore.h>
#include <signal.h>
#include <stdarg.h>
#include <sys/time.h>
#include "main_functionality.h" 

volatile sig_atomic_t active=1;

void handle_signal()
{
    active=0;
}

void handle_sigusr1()
{
    send_stats();
}


void handle_sigusr2()
{
    if(level.num==3)
        level.num=0;
    else
        level.num=level.num+1;
    setloglevel(level.num);
}

double get_total_processtime()
{
    int counter;
    double total_time;
    for(counter=0;counter<ind;counter++)
    {
        total_time = total_time + process_times[counter];
    }

    return total_time;
}

int send_stats()
{
    time_t curr_time;
    double time_diff;
    curr_time=time(NULL);
    char *lev =malloc(5);
    strcpy(lev,"DATA");
    if(strategy == 'w')
        request_count = threadp_requests;
    
    time_diff=difftime(curr_time,server_start_time);
    double processtime_total=get_total_processtime();
    logt(lev,"\n\nSettings:---------------\n");
    logt(lev,"Document Root   : %s\n",directory_name);
    logt(lev,"Port No         : %d\n", portno);
    switch(strategy)
    {
       case 's' : logt(lev,"Strategy        : Serial\n");
                  break;
 
        case 'f': logt(lev,"Strategy        : Fork\n");
                  break;

        case 't': logt(lev,"Strategy        : Thread\n");
		  break;

        case 'w': logt(lev,"Strategy        : Thread Pool\n");
	          logt(lev,"Thread Pool Size: %d\n",threadno);
		  logt(lev,"BUffer Size     : %d\n",buffersize);
		  break;
    }
    logt(lev,"Log Level       : %s\n",level.name);
   
    logt(lev,"\n\nStatistics-----------------------------------\n");
    logt(lev,"Total Uptime                      : %f secs\n",time_diff);
    logt(lev,"# of Requests handled             : %d\n",request_count);
    logt(lev,"Toal Processing Time              : %f secs\n",processtime_total/1000000);
    logt(lev,"Average Processing Time           : %f secs\n",(processtime_total/request_count)/1000000);
    logt(lev,"Total Amount of Data transfered   : %d bytes\n",total_bytes);
    
    free(lev);

    return 0;

}

int store_loglevel(char * loglevel)
{
    if(strcmp(loglevel,"ERROR")==0)   level.num = 0;
    if(strcmp(loglevel,"WARNING")==0) level.num = 1;
    if(strcmp(loglevel,"INFO")==0)    level.num = 2;
    if(strcmp(loglevel,"DEBUG")==0)   level.num = 3; 
    
    strcpy(level.name,loglevel); 
    logt("INFO","%s\n",level.name);
    return 0;
}



/*Function to find the content length*/

int find_content_length(char *filepath, http_status_t *http_status)
{
     struct stat stfile;
     int content_length=0;
     FILE *fpread=fopen(filepath,"r");
     if(fpread==NULL)
     {
         http_status->code=HTTP_STATUS_LOOKUP[POSITION_NOTFOUND].code;
         http_status->reason=HTTP_STATUS_LOOKUP[POSITION_NOTFOUND].reason;
     }
     else
     {
         int statcheck=stat(filepath,&stfile);
	 if(statcheck==0)
         {
             content_length=stfile.st_size;
         }
         else
         {
             http_status->code=HTTP_STATUS_LOOKUP[POSITION_INTERNALERROR].code;
             http_status->reason=HTTP_STATUS_LOOKUP[POSITION_INTERNALERROR].reason;
         }
	 fclose(fpread);  
     }
     
     pthread_mutex_lock(&mtx_bytes);
     total_bytes=total_bytes + content_length;
     pthread_mutex_unlock(&mtx_bytes);

     return content_length;
}

char getinput(int argc, char *argv[],char * directory_name)
{
    int c,check_fork,check_thread,check_threadpool;;
    char strategy='s';
    extern char * optarg;
    int default_port=9000;
    char * loglevel = malloc(BSIZE);
    memset(loglevel,0,BSIZE);

    while((c=getopt(argc,argv,"ftp:d:q:w:v:"))!=-1)
    switch(c) 
    { 
        case 'p': if(optarg!=NULL)
                      portno=atoi(optarg);
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
                          logt("DEBUG","Directory Does Not Exist\n");
                          exit(EXIT_FAILURE);
                      }
                  }
                  break;
        case 'w': if(optarg!=NULL)
                    threadno=atoi(optarg);
                  strategy='w';
                  check_threadpool=1;
                  break;

        case 'q': if(optarg!=NULL)
                    buffersize=atoi(optarg);
                  break;

        case 'v': if(optarg!=NULL)
                    strcpy(loglevel,optarg);
                  break;

        case '?': logt("ERROR","Invalid Arguments!\n");
                exit(EXIT_FAILURE);
    }

    if((check_fork==1 && check_thread==1)||(check_thread==1 && check_threadpool==1)||(check_fork==1 && check_threadpool==1))
    {
        logt("ERROR","Inappropriate Options\n");
        exit(EXIT_FAILURE);
    }    
    if(strcmp(loglevel,"")==0)
        strcpy(loglevel,"WARNING");
    store_loglevel(loglevel);

    if(buffersize==0)
        buffersize=BSIZE;

    if(portno==0)
        portno=default_port;

    free(loglevel);
    return strategy;
}

int dispatch_connection(char * read_request,char strategy,int fd,char * directory_name)
{
    switch(strategy)
    {
        case 's': dispatch_to_serial(read_request,fd,directory_name);
                  break;

        case 'f': dispatch_to_fork(read_request,fd,directory_name);
                  break;

        case 't': dispatch_to_thread(fd,directory_name);
                  break;
    }

    return 0;
}

static void * thread_func()
{
    char readbuf[MEMORYSIZE];
    thread_consumer(readbuf,buffersize,directory_name);
}


int dispatch_to_thread_pool(int sockfd,char *name)
{
    int newsockfd,counter,s;
    int no_of_threads=threadno;
    pthread_t tidarr[no_of_threads];
    memory_allocate(buffersize);
    
    sigset_t signew,sigold;
    sigemptyset(&signew);
    sigaddset(&signew,SIGINT);
    sigaddset(&signew,SIGTERM);
    sigaddset(&signew,SIGUSR1);
    sigaddset(&signew,SIGUSR2);
    pthread_sigmask(SIG_BLOCK,&signew,&sigold);

    for(counter=0;counter<no_of_threads;counter++)
    {
        s=pthread_create(&tidarr[counter],0,&thread_func,NULL);
    }
    
    pthread_sigmask(SIG_SETMASK,&sigold,NULL);
    while(active)
    {
        newsockfd=accept(sockfd,(struct sockaddr *)NULL,NULL);
        if(newsockfd<0)
        {
            continue;
        }
        threadp_requests++;
        if(newsockfd>0)
            thread_producer(directory_name,newsockfd,buffersize);
    }
    close(newsockfd);
    for(counter=0;counter<no_of_threads;counter++)
    {
        logt("DEBUG","Detaching thread\n");
        pthread_detach(tidarr[counter]);
    }
    free(buff_ptr);
    return 0;
}


int main(int argc, char * argv[])
{
    int sockfd;
    int newsockfd;
    char *read_request=malloc(MEMORYSIZE);
    long int port_num=0,default_port=9000;

    struct sigaction sa,sa_usr1,sa_usr2;
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
    
    sa_usr2.sa_flags=0;
    sa_usr2.sa_handler=handle_sigusr2;
    if(sigaction(SIGUSR2,&sa_usr2,NULL)==-1)
        exit(EXIT_FAILURE);

    memset(process_times,0,sizeof(process_times));
    strategy=getinput(argc,argv,directory_name); 
    logt("INFO","Proess Id:%d\n",getpid());

    sockfd=startserver(portno);
    time(&server_start_time);

    if(strategy=='w')
    {
        dispatch_to_thread_pool(sockfd,directory_name);
    }
    else
    {
        while(active)
        {
            newsockfd=accept(sockfd,(struct sockaddr*)NULL,NULL);
            if(newsockfd<0)
            {
                continue;
            }
            request_count++;
            if(newsockfd>0)
                dispatch_connection(read_request,strategy,newsockfd,directory_name);
        }
     }
	
    free(read_request);
    close(newsockfd);
    close(sockfd);
    logt("DEBUG","Graceful Exit\n");
    return(0);
}
