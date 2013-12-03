#include <stdio.h>
#include <stdlib.h>
#include "bounded_buffer.h"
#include <pthread.h>
#include <semaphore.h>
#include "strategies.h"

typedef struct buff buff_t;

static pthread_mutex_t mtx_buff       = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_buff_full  = PTHREAD_COND_INITIALIZER;
static pthread_cond_t cond_buff_empty = PTHREAD_COND_INITIALIZER;


int memory_allocate(int size)
{
     buff_ptr=(buff_t *)malloc(sizeof(buff_t)* size );
     buff_ptr->insert_position=0;
     buff_ptr->remove_position=0;

    return 0;
}

int thread_producer(char *name,int fd,int size)
{
    pthread_mutex_lock(&mtx_buff);
    int curr_position = (buff_ptr->insert_position + 1) % size;
    while(buff_ptr->remove_position==curr_position)
    {
        pthread_cond_wait(&cond_buff_full,&mtx_buff);
    }
    buff_ptr->bounded_buff[buff_ptr->insert_position]=fd;
    pthread_cond_broadcast(&cond_buff_empty);
    buff_ptr->insert_position=curr_position;
    pthread_mutex_unlock(&mtx_buff);

    return 0;
}

int thread_consumer(int size,char * dir_name)
{
    while(active)
    {
        pthread_mutex_lock(&mtx_buff);
        while(buff_ptr->remove_position == buff_ptr->insert_position)
        {
            pthread_cond_wait(&cond_buff_empty,&mtx_buff);
        }
        int curr_position = (buff_ptr->remove_position+1) % size;
        int fd = buff_ptr->bounded_buff[buff_ptr->remove_position]; 
        pthread_cond_broadcast(&cond_buff_full);
        buff_ptr->remove_position = curr_position;
        pthread_mutex_unlock(&mtx_buff);
        if(check)
        {
            bytes=read(fd,readbuf,MEMORYSIZE);
            process_request(readbuf,dir_name,fd);
        }
        close(fd);
    }
//    perror("out of threads");
    return 0;
}

