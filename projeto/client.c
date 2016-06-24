#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#define SERVER_PORT 56789
#define MAX_LINE 256

int main(int argc, char * argv[]){
    
    FILE *fp;
    struct hostent *hp;
    struct sockaddr_in sin, end;
    int addrlen = sizeof(end);
    char local_ip_buf[INET_ADDRSTRLEN]; /* buf size for IPV4 only */
    char *host;
    char *svport;
    char *username;
    char buf[MAX_LINE];
    char login_msg[MAX_LINE];
    int s;
    int len;
    int login = 0;
    int portnumber;
    
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
        strcpy(login_msg, "LOGIN ");
        strcat(login_msg, username);
        strcat(login_msg, "\n");
        login_msg[MAX_LINE-1] = '\0';
        len = strlen(login_msg) + 1;
        send(s, login_msg, len, 0);
        printf("$[%s]", username);
    }
    
    /* main loop: get and send lines of text */
    while (fgets(buf, sizeof(buf), stdin)) {
        buf[MAX_LINE-1] = '\0';
        len = strlen(buf) + 1;
        send(s, buf, len, 0);
        printf("$[%s]", username);
    }
}
