#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_PORT 56789
#define MAX_PENDING 5
#define MAX_LINE 256

int max(int, int);

int main()
{
    struct sockaddr_in cliaddr, servaddr;
    char buf[MAX_LINE];
    char peer_ip_buf[INET_ADDRSTRLEN]; /* buf size for IPV4 only */
    socklen_t clilen;
    ssize_t len;
    int listenfd, connfd, udpfd, maxfd;
    int nready, optval = 1;
    pid_t pid;
    fd_set allset, rset;
    
    /* build address data structure */
    bzero((char *)&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(SERVER_PORT);
    
    /* setup passive open */
    if ((listenfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("simplex-talk: socket");
        exit(1);
    }
    
    /* setup udp socket */
    if ((udpfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("simplex-talk: socket");
        exit(1);
    }
    
    if ((setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))) < 0) {
        perror("simplex-talk: setsockopt");
        exit(1);
    }
    
    if ((bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) < 0) {
        perror("simplex-talk: bind");
        exit(1);
    }
    
    if ((bind(udpfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) < 0) {
        perror("simplex-talk: bind");
        exit(1);
    }
    
    listen(listenfd, MAX_PENDING);
    
    maxfd = max(listenfd, udpfd) + 1;			/* initialize */
    FD_ZERO(&allset); /* Initializes the file descriptors of the file descriptors set allset with 0 bits */
    FD_SET(listenfd, &allset); /* Adds listenfd to the file descriptor set allset */
    FD_SET(udpfd, &allset);
    
    /* wait for connection, then receive and print text */
    while(1) {
        
        rset = allset;
        
        if((nready = select(maxfd+1, &rset, NULL, NULL, NULL)) == 0){
            perror("timeout error");
            close(listenfd);
            return 1;
        }
        
        if (FD_ISSET(listenfd, &rset)) {
            
            clilen = sizeof(cliaddr);
            if ((connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen)) < 0) {
                perror("simplex-talk: accept");
                close(listenfd);
                return 1;
            }
            
            /* Test for fork error */
            if ((pid = fork()) < 0) {
                perror("fork");
                close(connfd);
            }
            
            /* If the pid is 0 we are in the child process */
            else if (pid == 0) {
                
                close(listenfd);
                
                if (getpeername(connfd, (struct sockaddr*)&servaddr, (socklen_t*)&len) == 0) {
                    inet_ntop(AF_INET, &servaddr.sin_addr, peer_ip_buf, sizeof peer_ip_buf);
                    printf("Socket do cliente: \n");
                    printf("IP: #%s\n", peer_ip_buf);
                    printf("#Porta: #%d\n", ntohs(servaddr.sin_port));
                }
                
                while ((len = recv(connfd, buf, MAX_LINE, 0)) > 0){
                    fputs(buf, stdout);
                    send(connfd, buf, len, 0);
                }
                close(connfd);
                
                if (--nready <= 0)
                    continue;				/* no more readable descriptors */
            }
            close(connfd);
        }
        
        if (FD_ISSET(udpfd, &rset)) {
            
            clilen = sizeof(cliaddr);
            
            if((len = recvfrom(udpfd, buf, MAX_LINE, 0, (struct sockaddr *) &cliaddr, &clilen)) < 0){
                perror("socket error");
                close(udpfd);
                return 1;
            }
            
            printf("Pacote UDP recebido do IP: %s Porta: %d\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
            fputs(buf, stdout);
            
            if(sendto(udpfd, buf, strlen(buf) + 1, 0, (struct sockaddr *) &cliaddr, clilen) < 0) {
                perror("message error");
                close(udpfd);
                return 1;
            }
            
            if (--nready <= 0)
                continue;				/* no more readable descriptors */
        }
    }
}

int max(int a, int b){
    
    if (a > b)
        return a;
    else
        return b;
}
