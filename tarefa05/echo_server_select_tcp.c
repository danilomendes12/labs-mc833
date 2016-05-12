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

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {  /* Return socket file descriptor(positive integer) if successful  */
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

	if (listen(listenfd, LISTENQ) < 0) { /* Makes the socket willing to accept incoming connections, LISTENQ defines the maxlength for the queue of incoming connections */
		perror("listen error");
		close(listenfd);
		return 1;
	}

	maxfd = listenfd;			/* initialize */
	maxi = -1;					/* index into client[] array */
    for (i = 0; i < FD_SETSIZE; i++) /* FD_SETSIZE indicates the maximum number of file descriptors a FD_SET object can hold info about. If a fd is as high as FD_SETSIZE that fd cannot be put into an fd_set */
		client[i] = -1;			/* -1 indicates available entry */
	FD_ZERO(&allset); /* Initializes the file descriptors of the file descriptors set allset with 0 bits */
	FD_SET(listenfd, &allset); /* Adds listenfd to the file descriptor set allset */

	for ( ; ; ) {
		rset = allset;		/* structure assignment */
		nready = select(maxfd+1, &rset, NULL, NULL, NULL); /* Check tarefa05 exercicio1/ TL;DR.: returns the number of sockets ready for reading */
		
		if((nready = select(maxfd+1, &rset, NULL, NULL, NULL)) == 0){
			perror("timeout error");
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

			for (i = 0; i < FD_SETSIZE; i++)
				if (client[i] < 0) {
					client[i] = connfd;	/* save descriptor */
					break;
				}
			if (i == FD_SETSIZE) {
				perror("too many clients");
                exit(1);
            }

			FD_SET(connfd, &allset);	/* add new descriptor to set */
			if (connfd > maxfd)
				maxfd = connfd;			/* for select, used instead of FD_SETSIZE for better performance */
			if (i > maxi)
				maxi = i;				/* max index in client[] array */

			if (--nready <= 0)
				continue;				/* no more readable descriptors */
		}

		for (i = 0; i <= maxi; i++) {	/* check all clients for data */
			if ( (sockfd = client[i]) < 0) /* if there is no descriptor saved, continue */
				continue;
			if (FD_ISSET(sockfd, &rset)) { /* If socket sockfd is ready for reading */
				if ( (n = read(sockfd, buf, MAXLINE)) == 0) { /* read and puto message into buf. zero returned means end of connection */
						/* connection closed by client */
					close(sockfd);
					FD_CLR(sockfd, &allset); /* Removes sockfd from the file descriptor set allset. */
					client[i] = -1;
				} else
                    fputs(buf, stdout); /* NOT IN THE ORIGINAL FILE - MODIFIED BY GRIJÓ */
					send(sockfd, buf, n, 0); /* sends a message on a socket. s is the socket, buf is the message, n is the message length, 0 is the the flag that specifies the type of transmission. Successful completion of send does not guarantee the message was delivered. A return of -1 indicates only locally-detected errors */

				if (--nready <= 0) /* decrements the number of sockets ready for connections */
					if(send(sockfd, buf, n, 0) < 0){
						perror("message error");
						close(sockfd);
						return 1;
					}
				if (--nready <= 0)
					break;				/* no more readable descriptors */
			}
		}
	}
}
