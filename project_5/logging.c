#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdlib.h>

int logt(int log_level, const char * message,...)
{
    va_list arg_list;
    int stream;
    FILE *fp;
    switch(log_level)
    {
        case 0: fp=stderr;
                break;

        case 1: fp=stderr;
                break;

        case 2: fp=stdout;
                break;

        case 3: fp=stdout;
                break;
    }

    va_start(arg_list,message);
    vfprintf(fp,message,arg_list);
    va_end(arg_list);

    return 0;
}
