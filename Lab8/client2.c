//File sending through TCP - client side
//Commented
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <signal.h>
#include <netdb.h>

//Error macro
#define ERR(source) (perror(source),\
		     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
		     exit(EXIT_FAILURE))

//German error macro - used in task 1, but not here
#define HERR(source) (fprintf(stderr,"%s(%d) at %s:%d\n",source,h_errno,__FILE__,__LINE__),\
		     exit(EXIT_FAILURE))

//Defined size of the chunk, filename length
#define CHUNKSIZE 500
#define NMMAX 30

//Usage function (usually it's lower. huh)
void usage(char *name) {
	fprintf(stderr, "USAGE: %s domain port\n", name);
	exit(EXIT_FAILURE);
}

//Signal handling setup function)
void sethandler(void (*f)(int), int sigNo) {
	struct sigaction act;
	memset(&act, 0x00, sizeof(struct sigaction));
	act.sa_handler = f;

	if (-1 == sigaction(sigNo, &act, NULL))
		ERR("sigaction");
}

//Library function: bulk read
ssize_t bulk_read(int fd, char *buf, size_t count) {
	int c;
	size_t len = 0;

	do {
		c = TEMP_FAILURE_RETRY(read(fd, buf, count));
		if (c < 0)
			return c;
		if (c == 0)
			return len;
		buf += c;
		len += c;
		count -= c;
	}
	while (count > 0);
	return len;
}

//Library function: bulk write
ssize_t bulk_write(int fd, char *buf, size_t count) {
	int c;
	size_t len = 0;
	do {
		c = TEMP_FAILURE_RETRY(write(fd, buf, count));
		if(c < 0)
			return c;
		buf += c;
		len += c;
		count -= c;
	}
	while (count > 0);
	return len;
}

//Create a TCP-IP socket
int make_socket(void) {
	int sock;
	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock < 0)
        ERR("socket");
	return sock;
}

//Make computer-readable address from provided ones (using getaddrinfo())
struct sockaddr_in make_address(char *address, char *port){
        int ret;
        struct sockaddr_in addr;
        struct addrinfo *result;
        struct addrinfo hints = {};
        hints.ai_family = AF_INET;
        if((ret=getaddrinfo(address,port, &hints, &result))){
                fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
                exit(EXIT_FAILURE);
        }
        addr = *(struct sockaddr_in *)(result->ai_addr);
        freeaddrinfo(result);
        return addr;
}

//Connect the socket
int connect_socket(char *name, char *port) {
	struct sockaddr_in addr;
	int socketfd;
    //Create the socket
	socketfd = make_socket();
    //Prepare address to connect to
	addr = make_address(name, port);
    //Try to connect
	if (connect(socketfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) < 0)
	{
		if (errno != EINTR) //If failed NOT due to SIGINT - exit
            ERR("connect");
        //Otherwise try to check if SIGINT did any harm to connect()
		else { 
			fd_set wfds;
			int status;
			socklen_t size = sizeof(int);
			FD_ZERO(&wfds);
			FD_SET(socketfd, &wfds);
            //Try to check if socket is writeable (if any) - breaks if error occurs
			if (TEMP_FAILURE_RETRY(select(socketfd + 1, NULL, &wfds, NULL, NULL)) < 0)
                ERR("select");
            //Get error value on the socket
			if (getsockopt(socketfd, SOL_SOCKET, SO_ERROR, &status, &size) < 0)
                ERR("getsockopt");
            //If any occurred - break
			if (status != 0)
                ERR("connect");
		}
	}
    //Return connected socket
	return socketfd;
}

//Communicate with server
void communicate(int fd) {
	char file[NMMAX + 1];
	char buf[CHUNKSIZE + 1];
    //Fill data for buffer and filename
	memset(file, 0x00, NMMAX + 1);
	memset(buf, 0x00, CHUNKSIZE + 1);
    //Get filename
	fgets(file, NMMAX + 1, stdin);
    //Properly end the string - replace newline with null terminator
	if (file[strlen(file) - 1] == '\n')
		file[strlen(file) - 1] = '\0';
    //Write filename to socket
	if (bulk_write(fd, (void *) file, NMMAX + 1) < 0)
        ERR("write");
    //Read file from socket
	if (bulk_read(fd, (void *) buf, CHUNKSIZE) < 0)
        ERR("read");
    //Close the socket
	if (TEMP_FAILURE_RETRY(close(fd)) < 0)
        ERR("close");
	printf("%s", buf);
}

int main(int argc, char **argv) {
	int fd;
	if (argc!=3) usage(argv[0]);
	sethandler(SIG_IGN, SIGPIPE); //Ignore SIGPIPE
    //Connect socket
	fd = connect_socket(argv[1],argv[2]);
    //Communicate with server
	communicate(fd);
	return EXIT_SUCCESS;
}
