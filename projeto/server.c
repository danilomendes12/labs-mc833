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
#define MAX_STORED_MESSAGES 50

typedef struct client{ /* represents a client */
    int fd;
    char username[MAXLINE];
    bool status;    /* false: offline, true: online */
}client;

typedef struct stored_message{ /* represents a client */
    char message[MAXLINE];
    char username_from[MAXLINE];
    char username_to[MAXLINE];    /* false: offline, true: online */
}stored_message;

int getClientData(client *, char *, int);
int isSendMessage(char[]);
int isJoinGroup(char[]);
int isCreateGroup(char[]);
int isSendGroup(char[]);
void sendMessageTo(client[], char[],char[],char[]);
const char * getUsernameFromFd(client[], int);
int getFdFromUsername(client[], char[]);
int createGroup(client[], char[], char[]);
int joinGroup(client[], char[], char[]);
int sendGroupMessage(client[], char[], char[], char[]);
client getClientFromUsername(client[], char[]);
void sendGroupMessageTo(client[], char[], char[], char[], char[]);
unsigned long hash(unsigned char *);



client              chatgroups[10][FD_SETSIZE];
char                chatgroupnames[10][MAXLINE];
stored_message      stored_messages[MAX_STORED_MESSAGES];


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
    char                send_msg[MAXLINE];
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

    for(int j = 0; j < MAX_STORED_MESSAGES; j++){
        strcpy(stored_messages[j].username_from, "");
        strcpy(stored_messages[j].username_to, "");
        strcpy(stored_messages[j].message, "");
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

                        strcpy(send_msg, "");
                        while(token != NULL){
                            strcat (send_msg, token);
                            strcat(send_msg, " ");
                            token = strtok(NULL, s);
                        }

                        send_msg[(strlen(send_msg)-1)] = '\0';
                        
                        //send message
                        sendMessageTo(clients, aux_emitter,send_msg,receiver);
                        
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
                        
                        int has_created = createGroup(clients, groupname, aux_emitter);

                        if(has_created == 1){
                            send(sockfd, "Grupo criado com sucesso\n", sizeof("Grupo criado com sucesso\n"), 0);
                        }else if(has_created == 2){
                            send(sockfd, "Ja existe grupo com esse nome\n", sizeof("Ja existe grupo com esse nome\n"), 0);
                        } else {
                            send(sockfd, "Grupo criado com sucesso\n", sizeof("Grupo criado com sucesso\n"), 0);
                        }
                        
                        
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
                        
                        int has_joined = joinGroup(clients, groupname, aux_emitter);

                        if(has_joined == 1){
                            send(sockfd, "Entrou no grupo com sucesso\n", sizeof("Entrou no grupo com sucesso\n"), 0);
                        }else{
                            send(sockfd, "Não foi possivel entrar no grupo\n", sizeof("Não foi possivel entrar no grupo\n"), 0);
                        }
                        
                        
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
                        
                        strcpy(send_msg, "");
                        while(token != NULL){
                            strcat (send_msg, token);
                            strcat(send_msg, " ");
                            token = strtok(NULL, s);
                        }

                        send_msg[(strlen(send_msg)-1)] = '\0';

                        int has_sent = sendGroupMessage(clients, groupname, aux_emitter, send_msg);
                        if(has_sent == 1){
                            send(sockfd, "Enviou mensagem no grupo com sucesso\n", sizeof("Enviou mensagem no grupo com sucesso\n"), 0);
                        }else{
                            send(sockfd, "Grupo nao foi encontrado\n", sizeof("Grupo nao foi encontrado\n"), 0);
                        }
                        
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
                        send(sockfd, "Mensagem invalida\n", sizeof("Mensagem invalida\n"), 0);
                    }
                }
                if (--nready <= 0)
                    break;
            }
        }
    }
}

int createGroup(client clients[FD_SETSIZE], char groupname[MAXLINE], char creator_username[MAXLINE]){
    client creator = getClientFromUsername(clients, creator_username);
    
    for(int c = 0; c < MAXLINE; c++){
        if(groupname[c] == '\n') groupname[c] = '\0';
    }

    for(int i = 0; i < 10; i++){
        if(strcmp(chatgroupnames[i], groupname)==0){
            return 2;
        }
        if(chatgroupnames[i][0] == '\0'){
            chatgroups[i][0] = creator;
            strcpy(chatgroupnames[i], groupname);
            printf("%s criou o grupo %s\n", chatgroups[i][0].username, chatgroupnames[i]);
            return 1;
        }
    }

    return 0;
}

int joinGroup(client clients[FD_SETSIZE], char groupname[MAXLINE], char username[MAXLINE]){
    client join_client = getClientFromUsername(clients, username);
    
    for(int c = 0; c < MAXLINE; c++){
        if(groupname[c] == '\n') groupname[c] = '\0';
    }
    
    for(int i = 0; i < 10; i++){
        if(strcmp(groupname, chatgroupnames[i]) == 0){
            for(int x = 0; x < FD_SETSIZE; x++){
                if(chatgroups[i][x].fd == 0){
                    chatgroups[i][x] = join_client;
                    printf("Usuário %s entrou no grupo %s\n", chatgroups[i][x].username, chatgroupnames[i]);
                    return 1;
                }
            }
        }
    }

    return 0;
}

int sendGroupMessage(client clients[FD_SETSIZE], char groupname[MAXLINE], char username[MAXLINE], char message[MAXLINE]){
    
    for(int i = 0; i < 10; i++){
        if(strcmp(groupname, chatgroupnames[i]) == 0){
            for(int x = 0; x < FD_SETSIZE; x++){
                if(chatgroups[i][x].fd != 0) sendGroupMessageTo(clients, username, message, chatgroups[i][x].username, groupname);
            }
            return 1;
        }
    }

    return 0;
}


void sendGroupMessageTo(client clients[FD_SETSIZE], char username_from[MAXLINE], char message[MAXLINE], char username_to[MAXLINE], char name[MAXLINE]){
    int targetFd = getFdFromUsername(clients, username_to);
    char send_message[MAXLINE] = "\n$[";
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

bool getStatusFromFd(client clients[FD_SETSIZE], int fd){
    for(int i = 0; i < FD_SETSIZE; i++){
        if(clients[i].fd == fd){
            return clients[i].status;
        }
    }
    return false;
}

void sendMessageTo(client clients[FD_SETSIZE], char username_from[MAXLINE], char message[MAXLINE], char username_to[MAXLINE]){
    char send_message[MAXLINE] = "\n$[";
    strcat(send_message, username_from);
    strcat(send_message, ">] ");
    strcat(send_message, message);
    fputs(send_message, stdout);

    int destFd = getFdFromUsername(clients, username_to);
    int srcFd = getFdFromUsername(clients, username_from);

    if(getStatusFromFd(clients, destFd) == true){
        send(destFd, send_message, sizeof(send_message), 0);
        send(srcFd, "Mensagem enviada com sucesso\n", sizeof("Mensagem enviada com sucesso\n"), 0);
    }else{
        for(int j = 0; j < MAX_STORED_MESSAGES; j++){
            if(strcmp(stored_messages[j].username_to, "") == 0){
                strcpy(stored_messages[j].username_to, username_to);
                strcpy(stored_messages[j].username_from, username_from);
                strcpy(stored_messages[j].message, message);
                break;
            }   
        }
        send(srcFd, "Usuario esta offline, mensagem armazenada no servidor\n", sizeof("Usuario esta offline, mensagem armazenada no servidor\n"), 0);
    }
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
    char send_message[MAXLINE] = "";
    
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

                for(int j = 0; j < MAX_STORED_MESSAGES; j++){
                    if(strcmp(stored_messages[j].username_to, clients[i].username) == 0){
                        strcat(send_message, "$[");
                        strcat(send_message, stored_messages[j].username_from);
                        strcat(send_message, ">] ");
                        strcat(send_message, stored_messages[j].message);
                        strcpy(stored_messages[j].username_from, "");
                        strcpy(stored_messages[j].username_to, "");
                        strcpy(stored_messages[j].message, "");
                    }
                }

                break;
            } else if (strcmp(clients[i].username, buf) == 0) { /* Old user logging in, checks if this is the correct position for this user */
                clients[i].fd = connfd;
                clients[i].status = true;
                printf("LOGIN %s\n", clients[i].username);
                strcpy(bufsend, "Welcome Back, ");
                strcat(bufsend, clients[i].username);
                strcat(bufsend, "!\n");

                for(int j = 0; j < MAX_STORED_MESSAGES; j++){
                    if(strcmp(stored_messages[j].username_to, clients[i].username) == 0){
                        strcat(send_message, "$[");
                        strcat(send_message, stored_messages[j].username_from);
                        strcat(send_message, ">] ");
                        strcat(send_message, stored_messages[j].message);
                        strcpy(stored_messages[j].username_from, "");
                        strcpy(stored_messages[j].username_to, "");
                        strcpy(stored_messages[j].message, "");
                    }
                }
                break;
            }
        } else if (strcmp(clients[i].username, buf) == 0) { /* Check if the username is already taken */
            strcpy(bufsend, "Username taken, try anoter one.\n");
            break;
        }
    }

    if(strcmp(send_message, "") != 0){
        send(connfd, send_message, sizeof(send_message), 0);
    }
    send(connfd, bufsend, sizeof(bufsend), 0);
    
    return i;
}
