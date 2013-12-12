/******************************************************************************************************************************************

FILE NAME: logging.c
STUDENT NAME:Diana Rajan
COURSE: CSCI 640
PROF. NAME: CHRIS MORRIS

******************************************************************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "main_functionality.h" 


int setloglevel(int levnum)
{
    memset(level.name,0,sizeof(level.name));
    switch(levnum)
    {
	case 0: strncpy(level.name,"ERROR",5);
                break;
        case 1: strncpy(level.name,"WARNING",7);
                break;
        case 2: strncpy(level.name,"INFO",4);
                break;
        case 3: strncpy(level.name,"DEBUG",5);
	        break;
    }
}

int logt(char * log_level, const char * message,...)
{
    va_list arg_list;
    int stream;
    FILE *fp;
    va_start(arg_list,message);
    switch(level.num)
    {
        case 0: if(strcmp(log_level,"ERROR")==0)
		    vfprintf(stderr,message,arg_list);
		if(strcmp(log_level,"DATA")==0)
		    vfprintf(stdout,message,arg_list);
                break;

        case 1: if(strcmp(log_level,"ERROR")==0)
		    vfprintf(stderr,message,arg_list);
		if(strcmp(log_level,"WARNING")==0)
		    vfprintf(stderr,message,arg_list);
		if(strcmp(log_level,"DATA")==0)
		    vfprintf(stdout,message,arg_list);
                break;

        case 2: if(strcmp(log_level,"ERROR")==0)
                    vfprintf(stderr,message,arg_list);
                if(strcmp(log_level,"WARNING")==0)
                    vfprintf(stderr,message,arg_list);
		if(strcmp(log_level,"INFO")==0)
		    vfprintf(stdout,message,arg_list);
                if(strcmp(log_level,"DATA")==0)
                    vfprintf(stdout,message,arg_list);
                break;

        case 3: if(strcmp(log_level,"ERROR")==0)
                    vfprintf(stderr,message,arg_list);
                if(strcmp(log_level,"WARNING")==0)
                    vfprintf(stderr,message,arg_list);
                if(strcmp(log_level,"INFO")==0)
                    vfprintf(stdout,message,arg_list);
		if(strcmp(log_level,"DEBUG")==0)
		    vfprintf(stdout,message,arg_list);
                if(strcmp(log_level,"DATA")==0)
                    vfprintf(stdout,message,arg_list);
                break;
    }
    va_end(arg_list);
    return 0;
}


