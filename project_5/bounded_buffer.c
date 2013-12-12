/******************************************************************************************************************************************

FILE NAME: bounded_buffer.c
STUDENT NAME:Diana Rajan
COURSE: CSCI 640
PROF. NAME: CHRIS MORRIS

******************************************************************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include "main_functionality.h" 




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

int thread_consumer(char * readbuf,int size,char * dir_name)
{
    size_t bytes;
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
        bytes=read(fd,readbuf,MEMORYSIZE);
        process_request(readbuf,dir_name,fd);
    }
    return 0;
}


