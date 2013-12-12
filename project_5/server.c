/******************************************************************************************************************************************

FILE NAME: server.c
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
#include "server.h"


int startserver(int port)
{
    int sockfd;
    struct sockaddr_in serv_addr;

    memset(&serv_addr, '0', sizeof(serv_addr));
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd==ERROR)
    {
        logt("ERROR","Socket Creation Error\n");
        exit(EXIT_FAILURE);    
    }
    int option=1;
    if(setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&option,sizeof(option))<0)
    {
	logt("ERROR","Reuse Address Failed\n");
	exit(EXIT_FAILURE);
    }    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);
                            
    int bindreturn=bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if(bindreturn==ERROR)
    {
        logt("ERROR","Bind Error\n");
        exit(EXIT_FAILURE); 
    }
    int listenreturn=listen(sockfd, BACKLOG);
    if(listenreturn==ERROR)
    {
        logt("ERROR","Listen Error\n");
        exit(EXIT_FAILURE);
    }
    return sockfd;
}
