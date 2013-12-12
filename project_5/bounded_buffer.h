/******************************************************************************************************************************************

FILE NAME: bounded_buffer.h
STUDENT NAME:Diana Rajan
COURSE: CSCI 640
PROF. NAME: CHRIS MORRIS

******************************************************************************************************************************************/
#ifndef _BOUNDED_BUFFER_H
#define _BOUNDED_BUFFER_H

int memory_allocate(int size);
int thread_producer(char *name,int fd,int size);
int thread_consumer(char * readbuf,int size,char * dir_name);
#endif;
