#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define SERVER_PORT 31472
#define MAX_LINE 256

int main(int argc, char * argv[])
{
	FILE *fp;
	struct hostent *hp; /* hostent is a struct used to store info about a given host, such as host name, IPv4 address etc. */
	struct sockaddr_in sin; /* structure used by sockets to specify a local or remote address to connect a socket */
	char *host;
	char buf[MAX_LINE];
	int s;
	int len;
	if (argc==2) { /* if the argument counter is 2(that is, if there is another argument other than the program name, which is almost always prepended to the argv array) that argument will be attributed to host */
		host = argv[1];
	}
	else {
		fprintf(stderr, "usage: ./client host\n");
	exit(1);
    }

	/* translate host name into peer’s IP address */
	hp = gethostbyname(host); /* gethostbyname returns a pointer to an object with the hostent structure describing an internet host referenced by name. In this case, the host passed by argument */
	if (!hp) {
		fprintf(stderr, "simplex-talk: unknown host: %s\n", host);
		exit(1);
	}

	/* build address data structure */
	bzero((char *)&sin, sizeof(sin)); /* sets to zero the first sizeof(sin) bytes of the area starting at (char *)&sin */
	sin.sin_family = AF_INET; /* specifies that the socket will be used to connect to the internet with IPv4(AF_INET) */
	bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);  /* copy hp->h_length bytes from the area pointed by hp->h_addr to the area pointed by (char *)&sin.sin_addr */
	sin.sin_port = htons(SERVER_PORT); /* htons converts 2-byte quantities from host byte order to network byte order(big endian) */

	/* active open */
	if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) { /* Return socket file descriptor(positive integer) if successful  */
		perror("simplex-talk: socket");
		exit(1);
	}
  if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) { /* Attempts to make a connection on a socket. Returns 0 if successful  */
		perror("simplex-talk: connect");
		close(s);
		exit(1);
	}

	/* main loop: get and send lines of text */
	while (fgets(buf, sizeof(buf), stdin)) { /* if fgets is successful, returns buf(the string that was read) */
		buf[MAX_LINE-1] = '\0';
		len = strlen(buf) + 1;
		send(s, buf, len, 0); /* sends a message on a socket. s is the socket, buf is the message, len is the message length, 0 is the the flag that specifies the type of transmission. Successful completion of send does not guarantee the message was delivered. A return of -1 indicates only locally-detected errors */
	}
}
