/******************************************************************************************************************************************

FILE NAME: mimetypes.c
STUDENT NAME:Diana Rajan
COURSE: CSCI 640
PROF. NAME: CHRIS MORRIS

******************************************************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mimetypes.h"

#define PATHLEN 20;
#define COUNT   11;

int  parsepath(char * path,char *content_type)
{
    int i;
    char * path_type; 
    path_type = strrchr(path,'.');
    
    for(i = 0;i < 11;i++)
    {
	if(strcmp(path_type,MIME_TYPE_LOOKUP[i].ext)==0)
	{
	    	    
	    strncpy(content_type,MIME_TYPE_LOOKUP[i].contenttype,strlen(MIME_TYPE_LOOKUP[i].contenttype));
	    break;
	}
    }
    return 0;
}

