#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define SERVER_PORT 56789
#define MAX_LINE 256

int main(int argc, char * argv[]){
    
    struct hostent *hp;
    struct sockaddr_in sin, rcv;
    char *host;
    char buf[MAX_LINE];
    int s;
    ssize_t len;
    socklen_t rcvlen;
    if (argc==2) {
        host = argv[1];
    }
    else {
        fprintf(stderr, "usage: ./client host\n");
        exit(1);
    }
    
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
    sin.sin_port = htons(SERVER_PORT);
    
    /* active open */
    if ((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("simplex-talk: socket");
        exit(1);
    }
    
    /* main loop: get and send lines of text */
    while (fgets(buf, sizeof(buf), stdin)) {
        
        buf[MAX_LINE-1] = '\0';
        sendto(s, buf, strlen(buf) + 1, 0, (struct sockaddr *) &sin, sizeof(sin));
        rcvlen = sizeof(rcv);
        
        while ((len = recvfrom(s, buf, MAX_LINE, 0, (struct sockaddr *) &rcv, &rcvlen)) > 0) {
            
            if (ntohs(rcv.sin_port) == SERVER_PORT) {
                printf("Pacote UDP recebido do IP: %s Porta: %d\n", inet_ntoa(rcv.sin_addr), ntohs(rcv.sin_port));
                fputs(buf, stdout);
                break;
            }
        }
    }
}
