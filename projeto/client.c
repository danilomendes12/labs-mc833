#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

#define SERVER_PORT 56789
#define MAX_LINE 256

//void unblockFiles(FILE *);
//void unblockFileDescriptors(int);

int main(int argc, char * argv[]){
    
    struct hostent *hp;
    struct sockaddr_in sin;
    char *host;
    char *svport = NULL;
    char *username;
    char buf[MAX_LINE];
    char recv_msg[MAX_LINE];
    char login_msg[MAX_LINE];
    int s;
    long len;
    int portnumber, nready, maxfd;
    fd_set rset, allset;
    
    if (argc==4) {
        host = argv[1];
        svport = argv[2];
        username = argv[3];
    }
    else {
        fprintf(stderr, "usage: ./client host port username\n");
        exit(1);
    }
    
    portnumber = atoi(svport);
    
    /* translate host name into peer’s IP address */
    hp = gethostbyname(host);
    if (!hp) {
        fprintf(stderr, "simplex-talk: unknown host: %s\n", host);
        exit(1);
    }
    
    /* build address data structure */
    bzero((char *)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
    sin.sin_port = htons(portnumber);
    
    /* active open */
    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("simplex-talk: socket");
        exit(1);
    }
    
    if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        perror("simplex-talk: connect");
        close(s);
        exit(1);
    }else{
        strcat(login_msg, username);
        login_msg[MAX_LINE-1] = '\0';
        len = strlen(login_msg) + 1;
        send(s, login_msg, len, 0);
        if((len = recv(s, buf, sizeof(buf), 0)) > 0){
            fputs(buf, stdout);
            if (strcmp(buf, "Username taken, try anoter one.\n") == 0) {
                close(s);
                exit(1);
            }
        } else {
            perror("Server not found");
        }
        fprintf(stderr, "$[%s] ", username);
    }
    
    //    unblockFiles(stdin);
    //    unblockFileDescriptors(s);
    
    if (fileno(stdin) > s) maxfd = fileno(stdin);
    else maxfd = s;
    
    FD_ZERO(&allset); /* Initializes the file descriptors of the file descriptors set allset with 0 bits */
    FD_SET(s, &allset); /* Adds listenfd to the file descriptor set allset */
    FD_SET(fileno(stdin), &allset);
    
    /* main loop: get and send lines of text */
    while(1){
        
        rset = allset;		/* structure assignment */
        
        //        printf("$[%s] ", username);
        
        if((nready = select(maxfd+1, &rset, NULL, NULL, NULL)) == 0){
            close(s);
            return 1;
        }
        
        if (FD_ISSET(fileno(stdin), &rset)) {
            
            if(fgets(buf, sizeof(buf), stdin) != NULL) {
                buf[MAX_LINE-1] = '\0';
                len = strlen(buf) + 1;
                send(s, buf, len, 0);
            }
            if (fileno(stdin) > maxfd) maxfd = fileno(stdin);
            //            fprintf(stderr, "$[%s] ", username);
            
            if (--nready <= 0)
                continue;				/* no more readable descriptors */
        }
        
        if (FD_ISSET(s, &rset)) {
            if((len = recv(s, recv_msg, sizeof(recv_msg), 0)) > 0){
                fputs(recv_msg, stdout);
                if (strcmp(recv_msg, "LOGOUT\n") == 0) {
                    close(s);
                    exit(1);
                }
            }
            fprintf(stderr, "$[%s] ", username);
            if (s > maxfd) maxfd = s;
            
            if (--nready <= 0)
                continue;				/* no more readable descriptors */
            
        }
        
    }
}

//void unblockFiles(FILE *file) {
//
//    int flags;
//
//    flags = fcntl(fileno(stdin), F_GETFL, 0);
//    flags |= O_NONBLOCK;
//    fcntl(fileno(stdin), F_SETFL, flags);
//}
//
//void unblockFileDescriptors(int fd){
//
//    int flags;
//
//    flags = fcntl(fd, F_GETFL, 0);
//    flags |= O_NONBLOCK;
//    fcntl(fd, F_SETFL, flags);
//}
