//UDP server - task 2, lab 7
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <signal.h>
#include <netdb.h>
#include <fcntl.h>
//OLD: error macro
#define ERR(source) (perror(source),                                 \
					 fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), \
					 exit(EXIT_FAILURE))
//Number of awaiting connections
#define BACKLOG 3
//Maximum buffer size
#define MAXBUF 576
//Maximum number of connections
#define MAXADDR 5

//Connections structure - keep track of:
// - whether given struct is not filled
// - number of chunks received from connection
// - connection address
struct connections
{
	int free;
	int32_t chunkNo;
	struct sockaddr_in addr;
};

//OLD: handler setting function
int sethandler(void (*f)(int), int sigNo)
{
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (-1 == sigaction(sigNo, &act, NULL))
		return -1;
	return 0;
}

//Socket creation
int make_socket(int domain, int type)
{
	int sock;
	sock = socket(domain, type, 0);
	if (sock < 0)
		ERR("socket");
	return sock;
}

//Bind an internet socket (set it as listening)
int bind_inet_socket(uint16_t port, int type)
{
	struct sockaddr_in addr;
	int socketfd, t = 1;
	//Create a socket
	socketfd = make_socket(PF_INET, type);
	//Set up options
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;				  //Address from internet family
	addr.sin_port = htons(port);			  //Port as provided (network order, short)
	addr.sin_addr.s_addr = htonl(INADDR_ANY); //Attach to all devices (network order, long)
	//Allow for reusing addresses in a socket
	if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(t)))
		ERR("setsockopt");
	//Bind a socket (allow listening) using addr options
	if (bind(socketfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		ERR("bind");
	//If we use UDP (stream-oriented)
	if (SOCK_STREAM == type)
		//Listen for new connections
		if (listen(socketfd, BACKLOG) < 0)
			ERR("listen");
	return socketfd;
}

//Library function: bulk write
ssize_t bulk_write(int fd, char *buf, size_t count)
{
	int c;
	size_t len = 0;
	do
	{
		c = TEMP_FAILURE_RETRY(write(fd, buf, count));
		if (c < 0)
			return c;
		buf += c;
		len += c;
		count -= c;
	} while (count > 0);
	return len;
}

//Find an address in array of connections (connections struct comes to use)
int findIndex(struct sockaddr_in addr, struct connections con[MAXADDR])
{
	int i, empty = -1, pos = -1;
	//Compare to all entries in array
	for (i = 0; i < MAXADDR; i++)
	{
		//Get last empty element in array
		if (con[i].free)
			empty = i;
		//If we found an already-existent entry: break
		else if (0 == memcmp(&addr, &(con[i].addr), sizeof(struct sockaddr_in)))
		{
			pos = i;
			break;
		}
	}
	//If we have enough space and we didn't find address in array:
	//Set that free entry to this address
	if (-1 == pos && empty != -1)
	{
		con[empty].free = 0;
		con[empty].chunkNo = 0;
		con[empty].addr = addr;
		pos = empty;
	}
	//Return either found element, newly created element or -1 (meaning no place for any)
	return pos;
}

//Server work function
void doServer(int fd)
{
	struct sockaddr_in addr;
	struct connections con[MAXADDR];
	char buf[MAXBUF];
	socklen_t size = sizeof(addr);
	;
	int i;
	int32_t chunkNo, last;
	//First: set all connection slots as free
	for (i = 0; i < MAXADDR; i++)
		con[i].free = 1;
	//Do always
	for (;;)
	{
		//Try to receive something from a socket
		if (TEMP_FAILURE_RETRY(recvfrom(fd, buf, MAXBUF, 0, &addr, &size) < 0))
			ERR("read:");
		//If it came from something in array (or new entry was created)
		if ((i = findIndex(addr, con)) >= 0)
		{
			//Get data from received message
			chunkNo = ntohl(*((int32_t *)buf));
			last = ntohl(*(((int32_t *)buf) + 1));
			//If we missed some chunks, it means messages were lost: ignore
			if (chunkNo > con[i].chunkNo + 1)
				continue;
			//If we got exactly the chunk we needed
			else if (chunkNo == con[i].chunkNo + 1)
			{
				//If this was the last chunk: print the chunk and free the slot
				//in array (kinda disconnecting, but not)
				if (last)
				{
					printf("Last Part %d\n%s\n", chunkNo, buf + 2 * sizeof(int32_t));
					con[i].free = 1;
				}
				//Otherwise print the chunk
				else
					printf("Part %d\n%s\n", chunkNo, buf + 2 * sizeof(int32_t));
				//One way or another: increase chunk number - doesn't matter if in else block
				con[i].chunkNo++;
			}
			//Try to send the message back to client
			if (TEMP_FAILURE_RETRY(sendto(fd, buf, MAXBUF, 0, &addr, size)) < 0)
			{
				//If EPIPE happens (no connection anymore): free the slot
				if (EPIPE == errno)
					con[i].free = 1;
				//Otherwise something actually important happened, so quit
				else
					ERR("send:");
			}
		}
	}
}

//Usage function
void usage(char *name)
{
	fprintf(stderr, "USAGE: %s port\n", name);
}

int main(int argc, char **argv)
{
	int fd;
	//Check arguments
	if (argc != 2)
	{
		usage(argv[0]);
		return EXIT_FAILURE;
	}
	//Ignore SIGPIPE (as they aren't fatal here)
	if (sethandler(SIG_IGN, SIGPIPE))
		ERR("Seting SIGPIPE:");
	//Get file descriptor of the socket
	fd = bind_inet_socket(atoi(argv[1]), SOCK_DGRAM);
	//Do server work
	doServer(fd);
	//THIS STUFF NEVER GETS EXECUTED
	//YOU COULD GET THERE BY SIGINT HANDLING METHOD

	//Anyway: close socket file descriptor
	if (TEMP_FAILURE_RETRY(close(fd)) < 0)
		ERR("close");
	fprintf(stderr, "Server has terminated.\n");
	return EXIT_SUCCESS;
}
