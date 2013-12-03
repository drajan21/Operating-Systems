#ifndef _BOUNDED_BUFFER_H_
#define _BOUNDED_BUFFER_H_

#define SZ 16
#define MEMORYSIZE 1024

struct buff{
        int insert_position;
        int remove_position;
        int bounded_buff[];
       }*buff_ptr;

ssize_t bytes;
char    readbuf[MEMORYSIZE];
char *  read_request;


int insert_buffer(int insert_position,int fd_val);
int delete_buffer(int delete_poistion);
int display_buffer();
int memory_allocate(int size);
int thread_producer(char *name,int fd,int size);
int thread_consumer(int size,char * dir_name);
int displayval(int fd_ptr);
#endif
