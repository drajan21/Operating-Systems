#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "server.h"

#define BACKLOG 10

int startserver(int port)
{
    int sockfd;
    struct sockaddr_in serv_addr;

    memset(&serv_addr, '0', sizeof(serv_addr));
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd==ERROR)
    {
        perror("Socket Creation Error");
        exit(EXIT_FAILURE);    
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);
                            
    int bindreturn=bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if(bindreturn==ERROR)
    {
        perror(NULL);
        exit(EXIT_FAILURE); 
    }
    int listenreturn=listen(sockfd, BACKLOG);
    if(listenreturn==ERROR)
    {
        perror("Listen Error:");
        exit(EXIT_FAILURE);
    }
    return sockfd;
}
