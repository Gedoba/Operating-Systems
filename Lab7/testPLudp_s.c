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
#define ERR(source) (perror(source),\
		     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
		     exit(EXIT_FAILURE))
#define BACKLOG 3
#define MAXBUF 100
#define MAXADDR 5
volatile sig_atomic_t do_work=1;
int clientsCount = 0;


struct connections{
	int free;
	int32_t chunkNo;
	struct sockaddr_in addr;
};

void sigint_handler(int sig) {
	do_work=0;
}
int sethandler( void (*f)(int), int sigNo) {
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (-1==sigaction(sigNo, &act, NULL))
		return -1;
	return 0;
}
int make_socket(int domain, int type){
	int sock;
	sock = socket(domain,type,0);
	if(sock < 0) ERR("socket");
	return sock;
}
int bind_inet_socket(uint16_t port,int type){
	struct sockaddr_in addr;
	int socketfd,t=1;
	socketfd = make_socket(PF_INET,type);
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR,&t, sizeof(t))) ERR("setsockopt");
	if(bind(socketfd,(struct sockaddr*) &addr,sizeof(addr)) < 0)  ERR("bind");
	if(SOCK_STREAM==type)
		if(listen(socketfd, BACKLOG) < 0) ERR("listen");
	return socketfd;
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
int findIndex( struct sockaddr_in addr, struct connections con[MAXADDR]){
	int i,empty=-1,pos=-1;
	for(i=0;i<MAXADDR;i++){
		if(con[i].free) empty=i;
		else if(0==memcmp(&addr,&(con[i].addr),sizeof(struct sockaddr_in))) {
			pos=i;
			break;
		}
	}
	if(-1==pos&&empty!=-1) {
		con[empty].free=0;
		con[empty].chunkNo=0;
		con[empty].addr=addr;
		pos=empty;
	}
	return pos;
}


void doServer(int fd){
	struct sockaddr_in addr;
	struct connections con[MAXADDR];
	char buf[MAXBUF];
	socklen_t size=sizeof(addr);
	int i;
	double results[1];
	results[0] = -100; //if -100 this means division by zero occured
	for(i=0;i<MAXADDR;i++)con[i].free=1;
	while(do_work){
		if(TEMP_FAILURE_RETRY(recvfrom(fd,buf,MAXBUF,0,&addr,&size)<0)) {
				if(EINTR ==errno)
					continue;
				ERR("read:");
			
		}
    	clientsCount++;
		char ops[3];
    	ops[0] = '+';
    	ops[1] = '-';
    	ops[2] = '/';
    	buf[0] = '0' + rand() % 10;
    	buf[1] = ops[rand() % 3];
    	buf[2] = '0' + rand() % 10;
    	buf[3] = '\0';
		if(TEMP_FAILURE_RETRY(sendto(fd,buf,MAXBUF,0,&addr,size))<0){
			if(EPIPE==errno)
                con[i].free=1;
			else ERR("send:");
		
		}
		usleep(1500000);
		if(recvfrom(fd,results,sizeof(results),0,&addr,&size)<0){
			printf("retrying...\n");			
			recvfrom(fd,results,sizeof(results),0,&addr,&size);
		}
		if(results[0]!=-100)
			printf("\nresult: %lf\n", results[0]);
		else
			printf("\ndivision by zero occured\n");
		results [0] = -100;
		
	}
	
}
void usage(char * name){
	fprintf(stderr,"USAGE: %s port\n",name);
}
int main(int argc, char** argv) {
	int fd;
	if(argc!=2) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}
    srand(getpid());
	if(sethandler(SIG_IGN,SIGPIPE)) ERR("Seting SIGPIPE:");
	if(sethandler(sigint_handler,SIGINT)) ERR("Seting SIGINT:");	
	fd=bind_inet_socket(atoi(argv[1]),SOCK_DGRAM);
	doServer(fd);
	if(TEMP_FAILURE_RETRY(close(fd))<0)ERR("close");
	fprintf(stderr,"\nServer has terminated.\nClients: %d\n", clientsCount);
	return EXIT_SUCCESS;
}