#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define SERVER_PORT 31472
#define MAX_PENDING 5
#define MAX_LINE 256

int main()
{
	struct sockaddr_in sin;
	char buf[MAX_LINE];
	int len;
	int s, new_s;

	/* build address data structure */
	bzero((char *)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY; /* an IP is not specified, so the server can accept conections in any of the local host IP addresses. */
	sin.sin_port = htons(SERVER_PORT);

	/* setup passive open */
	if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) { /* creates socket */
		perror("simplex-talk: socket");
		exit(1);
	}
	if ((bind(s, (struct sockaddr *)&sin, sizeof(sin))) < 0) { /* binds the address specified by sockaddr to the socket s */
		perror("simplex-talk: bind");
		exit(1);
	}
	listen(s, MAX_PENDING); /* makes the socket s willing to accept incoming connections, MAX_PENDING defines the max length for the queue of pending connections. */

	/* wait for connection, then receive and print text */
	while(1) {
		if ((new_s = accept(s, (struct sockaddr *)&sin, &len)) < 0) { /* accepts the first connection on the queue of pending connections, create a new socket with the same socket type protocol and address family as the specified socket, and allocate a new file descriptor for that socket */
			perror("simplex-talk: accept");
			exit(1);
		}
		while (len = recv(new_s, buf, sizeof(buf), 0)) /* receives a message from a socket. Returns the length of the message */
			fputs(buf, stdout);
		close(new_s);
	}
}
