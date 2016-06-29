#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/select.h>
#include <string.h>
#include <stdbool.h>
#include <regex.h>


#define LISTENQ 5
#define MAXLINE 64
#define SERV_PORT 56789

typedef struct client{ /* represents a client */
    int fd;
    char username[MAXLINE];
    bool status;    /* false: offline, true: online */
}client;

int getClientData(client *, char *, int);
int isSendMessage(char[]);
int isJoinGroup(char[]);
int isCreateGroup(char[]);
int isSendGroup(char[]);
void sendMessageTo(client[], char[],char[],char[]);
const char * getUsernameFromFd(client[], int);
int getFdFromUsername(client[], char[]);
void createGroup(client[], char[], char[]);
void joinGroup(client[], char[], char[]);
void sendGroupMessage(client[], char[], char[], char[]);
client getClientFromUsername(client[], char[]);
void sendGroupMessageTo(client[], char[], char[], char[], char[]);



client              chatgroups[10][FD_SETSIZE];
char                chatgroupnames[10][MAXLINE];


int main(int argc, char **argv)
{
    int					i, j, maxi, maxfd, listenfd, connfd, sockfd;
    int                 client_amount; /* Actually client amount -1 */
    int					nready;
    ssize_t             n;
    client              clients[FD_SETSIZE];
    fd_set				rset, allset; /* fd_set represents fd sets for the select function. It's a bit array */
    char				buf[MAXLINE];
    char                msg_aux[MAXLINE];
    char                who_msg[MAXLINE];
    char                *svport = NULL;
    char                *token = NULL;
    socklen_t			clilen;
    struct sockaddr_in	cliaddr, servaddr;
    
    if (argc==2) {
        svport = argv[1];
    }else {
        fprintf(stderr, "usage: ./server port\n");
        exit(1);
    }
    
    for(int a = 0; a < 10; a++){
        for(int b = 0; b < FD_SETSIZE; b++){
            chatgroups[a][b].fd = 0;
        }
    }
    
    int portnumber = atoi(svport);
    
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {  /* Return socket fd(positive integer) if successful  */
        perror("socket error");
        return 1;
    }
    
    for(int x = 0; x < 10; x++){
        chatgroupnames[x][0] = '\0';
    }
    
    bzero(&servaddr, sizeof(servaddr)); /* Sets to zero the first sizeof(servaddr) bytes of memory starting at &servaddr */
    servaddr.sin_family      = AF_INET; /* Specifies that the address of the socket will be of IPV4 family */
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Specifies the socket will be binded to all local interfaces (ifconfig to check interfaces). htonl converts host byte order to network byte order) */
    servaddr.sin_port        = htons(portnumber);  /* Specifies the port of the socket. htons converts host byte order to network byte order */
    
    if (bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)  { /* binds the address and port specified by sockaddr to the socket s */
        perror("bind error");
        close(listenfd);
        return 1;
    }
    
    if (listen(listenfd, LISTENQ) < 0) { /* Makes the socket willing to accept incoming connections, LISTENQ defines the maxlength for the queue of incoming connections */
        perror("listen error");
        close(listenfd);
        return 1;
    }
    
    maxfd = listenfd;			/* initialize */
    maxi = -1;					/* index into client[] array */
    for (i = 0; i < FD_SETSIZE; i++){ /* FD_SETSIZE indicates the maximum number of file descriptors a FD_SET object can hold info about. If a fd is as high as FD_SETSIZE that fd cannot be put into an fd_set */
        clients[i].fd = -1;			/* -1 indicates available entry */
        strcpy(clients[i].username, "");
        clients[i].status = false;
    }
    FD_ZERO(&allset); /* Initializes the file descriptors of the file descriptors set allset with 0 bits */
    FD_SET(listenfd, &allset); /* Adds listenfd to the file descriptor set allset */
    
    for ( ; ; ) {
        rset = allset;		/* structure assignment */
        
        if((nready = select(maxfd+1, &rset, NULL, NULL, NULL)) == 0){
            close(listenfd);
            return 1;
        }
        
        if (FD_ISSET(listenfd, &rset)) {	/* new client connection */ /* FD_ISSET returns a nonzero value (true) if listenfd is a member of the file descriptor set rset, and zero (false) otherwise. In this case, true means the socket listenfd is ready for reading */
            clilen = sizeof(cliaddr);
            
            if((connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen)) < 0){
                perror("socket error");
                close(listenfd);
                return 1;
            }
            
            client_amount = getClientData(clients, buf, connfd);
            
            if (client_amount == FD_SETSIZE) {
                perror("too many clients");
                exit(1);
            }
            
            FD_SET(connfd, &allset);	/* add new descriptor to set */
            if (connfd > maxfd)
                maxfd = connfd;			/* for select, used instead of FD_SETSIZE for better performance */
            if (client_amount > maxi)
                maxi = client_amount;	/* max index in client[] array */
            
            if (--nready <= 0)
                continue;				/* no more readable descriptors */
        }
        
        for (i = 0; i <= maxi; i++) {	/* check all clients for data */
            if ( (sockfd = clients[i].fd) < 0) /* if there is no descriptor saved, continue */
                continue;
            if (FD_ISSET(sockfd, &rset)) { /* If sockfd is ready for reading */
                if ( (n = read(sockfd, buf, MAXLINE)) == 0) { /* read and puto message into buf. zero returned means end of connection */
                    /* connection closed by client */
                    close(sockfd);
                    FD_CLR(sockfd, &allset);
                    clients[i].fd = -1;
                    clients[i].status = false;
                    printf("LOGOUT %s\n", clients[i].username);
                    //                    strcpy(usernames[i], "");
                } else {
                    if ((isSendMessage(buf) == 1)) {
                        
                        const char *emitter = getUsernameFromFd(clients, sockfd);
                        char aux_emitter[MAXLINE];
                        strcpy(aux_emitter,emitter);
                        
                        char receiver[MAXLINE];
                        const char s[2] = " ";
                        strcpy(msg_aux, buf);
                        token = strtok(msg_aux, s);
                        token = strtok(NULL, s);
                        strcpy(receiver, token);
                        
                        
                        token = strtok(NULL, s);
                        strcpy (msg_aux, token);
                        
                        //send message
                        sendMessageTo(clients, aux_emitter,msg_aux,receiver);
                        send(sockfd, "Enviou mensagem com sucesso\n", sizeof("Enviou mensagem com sucesso\n"), 0);
                        
                    } else if ((isCreateGroup(buf) == 1)) {
                        
                        const char *emitter = getUsernameFromFd(clients, sockfd);
                        char aux_emitter[MAXLINE];
                        strcpy(aux_emitter,emitter);
                        
                        char groupname[MAXLINE];
                        const char s[2] = " ";
                        strcpy(msg_aux, buf);
                        token = strtok(msg_aux, s);
                        token = strtok(NULL, s);
                        strcpy(groupname, token);
                        
                        createGroup(clients, groupname, aux_emitter);
                        
                        send(sockfd, "Grupo criado com sucesso\n", sizeof("Grupo criado com sucesso\n"), 0);
                        
                    } else if ((isJoinGroup(buf) == 1)) {
                        
                        const char *emitter = getUsernameFromFd(clients, sockfd);
                        char aux_emitter[MAXLINE];
                        strcpy(aux_emitter,emitter);
                        
                        char groupname[MAXLINE];
                        const char s[2] = " ";
                        strcpy(msg_aux, buf);
                        token = strtok(msg_aux, s);
                        token = strtok(NULL, s);
                        strcpy(groupname, token);
                        
                        joinGroup(clients, groupname, aux_emitter);
                        
                        send(sockfd, "Entrou no grupo com sucesso\n", sizeof("Entrou no grupo com sucesso\n"), 0);
                        
                    } else if ((isSendGroup(buf) == 1)) {
                        
                        const char *emitter = getUsernameFromFd(clients, sockfd);
                        char aux_emitter[MAXLINE];
                        strcpy(aux_emitter,emitter);
                        
                        char groupname[MAXLINE];
                        const char s[2] = " ";
                        strcpy(msg_aux, buf);
                        token = strtok(msg_aux, s);
                        token = strtok(NULL, s);
                        strcpy(groupname, token);
                        
                        token = strtok(NULL, s);
                        strcpy (msg_aux, token);
                        
                        sendGroupMessage(clients, groupname, aux_emitter, msg_aux);
                        
                        send(sockfd, "Enviou mensagem no grupo com sucesso\n", sizeof("Enviou mensagem no grupo com sucesso\n"), 0);
                        
                    } else if (strcmp(buf, "WHO\n") == 0) {
                        
                        strcpy(who_msg, "USER | STATUS\n");
                        for (j = 0; j <= maxi; j++) {
                            strcat(who_msg, clients[j].username);
                            strcat(who_msg, " | ");
                            if (clients[j].status)
                                strcat(who_msg, "online\n");
                            else
                                strcat(who_msg, "offline\n");
                        }
                        send(sockfd, who_msg, sizeof(who_msg), 0);
                        
                    } else if (strcmp(buf, "EXIT\n") == 0) {
                        
                        send(sockfd, "LOGOUT\n", sizeof("LOGOUT\n"), 0);
                        close(sockfd);
                        FD_CLR(sockfd, &allset);
                        clients[i].fd = -1;
                        clients[i].status = false;
                        printf("LOGOUT %s\n", clients[i].username);
                    } else {
                        fputs(buf, stdout);
                        send(sockfd, "Message printed on the server\n", sizeof("Message printed on the server\n"), 0);
                    }
                }
                if (--nready <= 0)
                    break;
            }
        }
    }
}

void createGroup(client clients[FD_SETSIZE], char groupname[MAXLINE], char creator_username[MAXLINE]){
    client creator = getClientFromUsername(clients, creator_username);
    
    for(int c = 0; c < MAXLINE; c++){
        if(groupname[c] == '\n') groupname[c] = '\0';
    }
    
    for(int i = 0; i < 10; i++){
        if(chatgroupnames[i][0] == '\0'){
            chatgroups[i][0] = creator;
            strcpy(chatgroupnames[i], groupname);
            printf("%s\n", chatgroupnames[i]);
            printf("%s\n", chatgroups[i][0].username);
            return;
        }
    }
}

void joinGroup(client clients[FD_SETSIZE], char groupname[MAXLINE], char username[MAXLINE]){
    client join_client = getClientFromUsername(clients, username);
    
    for(int c = 0; c < MAXLINE; c++){
        if(groupname[c] == '\n') groupname[c] = '\0';
    }
    
    for(int i = 0; i < 10; i++){
        if(strcmp(groupname, chatgroupnames[i]) == 0){
            for(int x = 0; x < FD_SETSIZE; x++){
                if(chatgroups[i][x].fd == 0){
                    chatgroups[i][x] = join_client;
                    printf("%s\n", chatgroups[i][x].username);
                    printf("%s\n", chatgroupnames[i]);
                    return;
                }
            }
        }
    }
    
}

void sendGroupMessage(client clients[FD_SETSIZE], char groupname[MAXLINE], char username[MAXLINE], char message[MAXLINE]){
    
    for(int i = 0; i < 10; i++){
        if(strcmp(groupname, chatgroupnames[i]) == 0){
            for(int x = 0; x < FD_SETSIZE; x++){
                if(chatgroups[i][x].fd != 0) sendGroupMessageTo(clients, username, message, chatgroups[i][x].username, groupname);
            }
        }
    }
}


void sendGroupMessageTo(client clients[FD_SETSIZE], char username_from[MAXLINE], char message[MAXLINE], char username_to[MAXLINE], char name[MAXLINE]){
    int targetFd = getFdFromUsername(clients, username_to);
    char send_message[MAXLINE] = "[";
    strcat(send_message, username_from);
    strcat(send_message, "@");
    strcat(send_message, name);
    strcat(send_message, ">] ");
    strcat(send_message, message);
    fputs(send_message, stdout);
    send(targetFd, send_message, sizeof(send_message), 0);
}

client getClientFromUsername(client clients[FD_SETSIZE], char username[MAXLINE]){
    client aux = { -1, "", false};
    for(int i = 0; i < FD_SETSIZE; i++){
        if(strcmp(clients[i].username, username) == 0){
            return clients[i];
        }
    }
    return aux;
}

const char * getUsernameFromFd(client clients[FD_SETSIZE], int targetFd){
    for(int i = 0; i < FD_SETSIZE; i++){
        if(clients[i].fd == targetFd){
            return clients[i].username;
        }
    }
    return "";
}

int getFdFromUsername(client clients[FD_SETSIZE], char username[MAXLINE]){
    for(int i = 0; i < FD_SETSIZE; i++){
        if(strcmp(clients[i].username, username) == 0){
            return clients[i].fd;
        }
    }
    return 0;
}

void sendMessageTo(client clients[FD_SETSIZE], char username_from[MAXLINE], char message[MAXLINE], char username_to[MAXLINE]){
    int targetFd = getFdFromUsername(clients, username_to);
    char send_message[MAXLINE] = "\n[";
    strcat(send_message, username_from);
    strcat(send_message, ">] ");
    strcat(send_message, message);
    fputs(send_message, stdout);
    send(targetFd, send_message, sizeof(send_message), 0);
}

int isSendMessage(char send_msg[MAXLINE]){
    char *token;
    char aux[MAXLINE];
    strcpy(aux, send_msg);
    const char s[2] = " ";
    token = strtok(aux, s);
    if(strcmp(token, "SEND") == 0){
        return 1;
    }else{
        return 0;
    }
}

int isCreateGroup(char send_msg[MAXLINE]){
    char *token;
    char aux[MAXLINE];
    strcpy(aux, send_msg);
    const char s[2] = " ";
    token = strtok(aux, s);
    if(strcmp(token, "CREATEG") == 0){
        return 1;
    }else{
        return 0;
    }
}

int isJoinGroup(char send_msg[MAXLINE]){
    char *token;
    char aux[MAXLINE];
    strcpy(aux, send_msg);
    const char s[2] = " ";
    token = strtok(aux, s);
    if(strcmp(token, "JOING") == 0){
        return 1;
    }else{
        return 0;
    }
}

int isSendGroup(char send_msg[MAXLINE]){
    char *token;
    char aux[MAXLINE];
    strcpy(aux, send_msg);
    const char s[2] = " ";
    token = strtok(aux, s);
    if(strcmp(token, "SENDG") == 0){
        return 1;
    }else{
        return 0;
    }
}

int getClientData(client *clients, char *buf, int connfd) {
    
    int i;
    ssize_t n = 0;
    char bufsend[MAXLINE];
    
    for (i = 0; i < FD_SETSIZE; i++) {
        
        if (n == 0) { /* Test to guarantee it will read only once */
            if ( (n = read(connfd, buf, MAXLINE)) == 0) {
                close(connfd);
                clients[i].fd = -1;
            }
        }
        
        if (clients[i].fd < 0) {
            
            if (strcmp(clients[i].username, "") == 0) { /* New User */
                clients[i].fd = connfd;
                clients[i].status = true;
                strcpy(clients[i].username, buf);
                printf("LOGIN %s\n", clients[i].username);
                strcpy(bufsend, "Welcome, ");
                strcat(bufsend, clients[i].username);
                strcat(bufsend, "!\n");
                break;
            } else if (strcmp(clients[i].username, buf) == 0) { /* Old user logging in, checks if this is the correct position for this user */
                clients[i].fd = connfd;
                clients[i].status = true;
                printf("LOGIN %s\n", clients[i].username);
                strcpy(bufsend, "Welcome Back, ");
                strcat(bufsend, clients[i].username);
                strcat(bufsend, "!\n");
                break;
            }
        } else if (strcmp(clients[i].username, buf) == 0) { /* Check if the username is already taken */
            strcpy(bufsend, "Username taken, try anoter one.\n");
            break;
        }
    }
    
    send(connfd, bufsend, sizeof(bufsend), 0);
    
    return i;
}
