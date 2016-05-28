#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>


#define SERVER_PORT 56789
#define MAX_LINE 256

int main(int argc, char * argv[]){
    
    struct hostent *hp;
    struct sockaddr_in sin, end;
    socklen_t addrlen = sizeof(end);
    char local_ip_buf[INET_ADDRSTRLEN]; /* buf size for IPV4 only */
    char *host, *protocol;
    char buf[MAX_LINE];
    int s;
    int len;
    if (argc==3) {
        host = argv[1];
        protocol = argv[2];
        
        if (strcmp(protocol,"udp") != 0 && strcmp(protocol, "tcp") != 0) {
            fprintf(stderr, "protocol must be udp or tcp\n");
            exit(1);
        }
    }
    else {
        fprintf(stderr, "usage: ./client host protocol\n");
        exit(1);
    }
    
    protocol[0] = toupper(protocol[0]);
    protocol[1] = toupper(protocol[1]);
    protocol[2] = toupper(protocol[2]);
    
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
    
    if (strcmp(protocol,"UDP") == 0) {
        if ((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
            perror("simplex-talk: socket");
            exit(1);
        }
    } else {
        if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
            perror("simplex-talk: socket");
            exit(1);
        }
    }
    
    if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        perror("simplex-talk: connect");
        close(s);
        exit(1);
    }
    
    if (getsockname(s, (struct sockaddr *) &end, &addrlen) == 0) {
        inet_ntop(AF_INET, &end.sin_addr, local_ip_buf, sizeof local_ip_buf);
        printf("Socket Local: ");
        printf("IP %s ", local_ip_buf);
        printf("#Porta %d\n", ntohs(end.sin_port));
    }
    
    /* main loop: get and send lines of text */
    while (fgets(buf, sizeof(buf), stdin)) {
        
        buf[MAX_LINE-1] = '\0';
        send(s, buf, strlen(buf) + 1, 0);
        
        if((len = recv(s, buf, MAX_LINE, 0)) > 0){
            printf("Pacote recebido do IP: %s Porta: %d\n", inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));
            printf("Mensagem: ");
            fputs(buf, stdout);
        } else {
            perror("Server not found");
        }
    }
}
