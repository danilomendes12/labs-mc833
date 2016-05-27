#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER_PORT 56789
#define MAX_LINE 256

int main(int argc, char * argv[]){
    
    struct hostent *hp;
    struct sockaddr_in sin;
    char *host;
    char buf[MAX_LINE];
    int s;
    ssize_t len;
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
    
    if (connect(s, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
        perror("simplex-talk: connect");
        close(s);
        exit(1);
    }
    
    /* main loop: get and send lines of text */
    while (fgets(buf, sizeof(buf), stdin)) {
        
        buf[MAX_LINE-1] = '\0';
        send(s, buf, strlen(buf) + 1, 0);
        
        if ((len = recv(s, buf, MAX_LINE, 0)) > 0) {
            printf("Pacote UDP recebido do IP: %s Porta: %d\n", inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));
            fputs(buf, stdout);
        } else {
            perror("Server not found");
        }
    }
}
