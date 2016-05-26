/*
 * Code from http://www.unpbook.com/
 *
 *
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/select.h>
#include <string.h>

#define LISTENQ 5
#define MAXLINE 64
#define SERV_PORT 56789

int
main(int argc, char **argv)
{
    int					i, maxi, maxfd, listenfd, connfd, sockfd;
    int					nready, client[FD_SETSIZE];
    ssize_t				n;
    fd_set				rset, allset; /* fd_set is a data type that represents file descriptor sets for the select function. It's actually a bit array */
    char				buf[MAXLINE];
    socklen_t			clilen;
    struct sockaddr_in	cliaddr, servaddr;
    int len;
    
    if ((listenfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {  /* Return socket file descriptor(positive integer) if successful  */
        perror("socket error");
        return 1;
    }
    
    bzero(&servaddr, sizeof(servaddr)); /* Sets to zero the first sizeof(servaddr) bytes of memory starting at &servaddr */
    servaddr.sin_family      = AF_INET; /* Specifies that the address of the socket will be of IPV4 family */
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Specifies the socket will be binded to all local interfaces (ifconfig to check interfaces). htonl converts host byte order to network byte order) */
    servaddr.sin_port        = htons(SERV_PORT);  /* Specifies the port of the socket. htons converts host byte order to network byte order */
    
    if (bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)  { /* binds the address and port specified by sockaddr to the socket s */
        perror("bind error");
        close(listenfd);
        return 1;
    }
    
    for ( ; ; ) {

        clilen = sizeof(cliaddr);
        
        if((len = recvfrom(listenfd, buf, MAXLINE, 0, (struct sockaddr *) &cliaddr, &clilen)) < 0){
            perror("socket error");
            close(listenfd);
            return 1;
        }
        
        fputs(buf, stdout);
        
        if(sendto(listenfd, buf, sizeof(buf), 0, (struct sockaddr *) &cliaddr, clilen) < 0) {
            perror("message error");
            close(sockfd);
            return 1;
        }
    }
}
