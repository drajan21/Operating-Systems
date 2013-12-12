/******************************************************************************************************************************************

FILE NAME: strategies.h
STUDENT NAME:Diana Rajan
COURSE: CSCI 640
PROF. NAME: CHRIS MORRIS

******************************************************************************************************************************************/
#ifndef _STRATEGIES_H
#define _STRATEGIES_H

static void * thread_exec(void * arg);
int dispatch_to_serial(char * read_request,int fd, char * directory_name);
int dispatch_to_fork(char * read_request,int fd, char * directory_name);
int dispatch_to_thread(int fd, char * name);
#endif;
