#define _GNU_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <signal.h>
#include <netdb.h>
#define ERR(source) (perror(source),\
		fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
		exit(EXIT_FAILURE))

int sethandler( void (*f)(int), int sigNo) {
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (-1==sigaction(sigNo, &act, NULL))
		return -1;
	return 0;
}
int make_socket(void){
	int sock;
	sock = socket(PF_INET,SOCK_STREAM,0);
	if(sock < 0) ERR("socket");
	return sock;
}
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
int connect_socket(char *name, char *port){
	struct sockaddr_in addr;
	int socketfd;
	socketfd = make_socket();
	addr=make_address(name,port);
	if(connect(socketfd,(struct sockaddr*) &addr,sizeof(struct sockaddr_in)) < 0){
		if(errno!=EINTR) ERR("connect");
		else { 
			fd_set wfds ;
			int status;
			socklen_t size = sizeof(int);
			FD_ZERO(&wfds);
			FD_SET(socketfd, &wfds);
			if(TEMP_FAILURE_RETRY(select(socketfd+1,NULL,&wfds,NULL,NULL))<0) ERR("select");
			if(getsockopt(socketfd,SOL_SOCKET,SO_ERROR,&status,&size)<0) ERR("getsockopt");
			if(0!=status) ERR("connect");
		}
	}
	return socketfd;
}
ssize_t bulk_read(int fd, char *buf, size_t count){
	int c;
	size_t len=0;
	do{
		c=TEMP_FAILURE_RETRY(read(fd,buf,count));
		if(c<0) return c;
		if(0==c) return len;
		buf+=c;
		len+=c;
		count-=c;
	}while(count>0);
	return len ;
}
ssize_t bulk_write(int fd, char *buf, size_t count){
	int c;
	size_t len=0;
	do{
		c=TEMP_FAILURE_RETRY(write(fd,buf,count));
		if(c<0) return c;
		buf+=c;
		len+=c;
		count-=c;
	}while(count>0);
	return len ;
}
void prepare_request(char **argv,int32_t data[5]){
	data[0]=htonl(atoi(argv[3]));
	data[1]=htonl(atoi(argv[4]));
	data[2]=htonl(0);
	data[3]=htonl((int32_t)(argv[5][0]));
	data[4]=htonl(1);
}
void print_answer(int32_t data[1]){
	if(ntohl(data[0]))
		printf("ID: %d\n", ntohl(data[0]));
	else printf("Sth went wrong\n");
}
void usage(char * name){
	fprintf(stderr,"USAGE: %s name domainAddress port \n",name);
}
int main(int argc, char** argv) {
	int fd;
	int32_t data[1]; // id
	char buff[10];
	strcpy(buff, argv[1]);
	if(argc!=4) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}
	if(sethandler(SIG_IGN,SIGPIPE)) ERR("Seting SIGPIPE:");
	fd=connect_socket(argv[2],argv[3]);
	//prepare_request(argv,data);
	if(bulk_write(fd,(char *)buff,sizeof(char[10]))<0) ERR("write:");
	printf("write\n");
	if(bulk_read(fd,(char *)data,sizeof(int32_t[1]))<(int)sizeof(int32_t[1])) ERR("read:");
	printf("read\n");
	print_answer(data);
	if(TEMP_FAILURE_RETRY(close(fd))<0)ERR("close");
	return EXIT_SUCCESS;
}