/******************************************************************************************************************************************

FILE NAME: mimetypes.h
STUDENT NAME:Diana Rajan
COURSE: CSCI 640
PROF. NAME: CHRIS MORRIS

******************************************************************************************************************************************/

#ifndef _MIMETYPES_H
#define _MIMETYPES_H

#define MAX_EXT_LENGTH 10;
#define COUNT 11;

typedef struct {
    char *ext;
    char * contenttype;
}mime_type;

const mime_type MIME_TYPE_LOOKUP[] = {
    { ".html" , "text/html"},
    { ".jpeg" , "image/jpeg"},
    { ".png"  , "image/png"},
    { ".css"  , "text/css"},
    { ".js"   , "application/javascript"},
    { ".xml"  , "application/xml"},
    { ".mp3"  , "audio/mpeg"},
    { ".mpeg" , "video/mpeg"},
    { ".mpg"  , "video/mpg"},
    { ".mp4"  , "video/mp4"},
    { ".mov"  , "video/quicktime"}
};

int  parsepath(char * path,char * path_type);

#endif
