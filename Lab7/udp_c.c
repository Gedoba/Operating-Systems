//UDP client, commented - task 2, lab 7
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
#include <sys/stat.h>
#include <fcntl.h>

//OLD: error macro
#define ERR(source) (perror(source),\
		     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
		     exit(EXIT_FAILURE))
//Maximum buffer size
#define MAXBUF 576

//SIGALARM handling zone
volatile sig_atomic_t last_signal=0 ;
void sigalrm_handler(int sig) {
	last_signal=sig;
}
int sethandler( void (*f)(int), int sigNo) {
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (-1==sigaction(sigNo, &act, NULL))
		return -1;
	return 0;
}
//End of SIGALARM handling

//Socket creation function - PF_INET (internet family), SOCK_DGRAM (datagram), 0 (UDP)
int make_socket(void){
	int sock;
	sock = socket(PF_INET,SOCK_DGRAM,0);
	if(sock < 0) 
		ERR("socket");
	return sock;
}

//Address creation function
struct sockaddr_in make_address(char *address, char *port){
	int ret;
	struct sockaddr_in addr;
	struct addrinfo *result;
	struct addrinfo hints = {};
	//Family of address: IP
	hints.ai_family = AF_INET;
	//Try to get the address into result
	if((ret=getaddrinfo(address,port, &hints, &result))){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
		exit(EXIT_FAILURE);
	}
	//Get address from result structure
	addr = *(struct sockaddr_in *)(result->ai_addr);
	//Free result structure
	freeaddrinfo(result);
	//Return address
	return addr;
}

//Library function: bulk read
//tl;dr: read count until EOF is met or problem occurred
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

//Usage function
void usage(char * name){
	fprintf(stderr,"USAGE: %s domain port file \n",name);
}

//Send and confirm (this is UDP, this isn't automatic)
void sendAndConfirm(int fd,struct sockaddr_in addr,char *buf1, char *buf2, ssize_t size){
	struct itimerval ts;
	//Try to send a message to an address
	if(TEMP_FAILURE_RETRY(sendto(fd,buf1,size,0,&addr,sizeof(addr)))<0) 
		ERR("sendto:");
	//Set timer for reaction
	memset(&ts, 0, sizeof(struct itimerval));
	ts.it_value.tv_usec=500000;
	setitimer(ITIMER_REAL,&ts,NULL);
	last_signal=0;
	//Try to get a confirmation on socket (it gets stuck unless something gets received
	//or error occurs)
	while(recv(fd,buf2,size,0)<0){
		//If function didn't fail due to interruption: kill
		if(EINTR!=errno)
			ERR("recv:");
		//If function was interrupted with SIGALARM: break (and return) - time has elapsed
		if(SIGALRM==last_signal) 
			break;
		//Otherwise (e.g. SIGINT happened): continue receiving
	}
}

//Client function
void doClient(int fd, struct sockaddr_in addr, int file){
	char buf[MAXBUF];
	char buf2[MAXBUF];
	//This is offset from chunk and last part data
	int offset = 2*sizeof(int32_t);
	int32_t chunkNo=0;
	int32_t last=0;
	ssize_t size;
	int counter;
	//Do this as long as we read whole datagrams from socket
	do{
		//Try to read a datagram into a buffer (leaving place for chunk and last int)
		if((size=bulk_read(file,buf+offset,MAXBUF-offset))<0)
			ERR("read from file:");
		//Set chunk number to buffer
		*((int32_t*)buf)=htonl(++chunkNo);
		//If we read last datagram - set this chunk as last
		if(size<MAXBUF-offset) {
			last=1;
			//Also - nullify everything after datagram to avoid garbage values
			memset(buf+offset+size,0,MAXBUF-offset-size);
		}
		//Set if chunk is last (in network order)
		*(((int32_t*)buf)+1)=htonl(last);
		//Prepare buffer for receiving
		memset(buf2,0,MAXBUF);
		counter=0;
		//Do this until five send and confirmation attempts happened or we received back enough chunks
		do{
			//Count chunks sent
			counter++;
			//Try to send buffer, receive in buffer 2
			sendAndConfirm(fd,addr,buf,buf2,MAXBUF);
		}while(*((int32_t*)buf2)!=htonl(chunkNo)&&counter<=5);
		//If we did all send and confirms and we didn't receive back right number
		//of chunks: break the loop - something's wrong
		if(*((int32_t*)buf2)!=htonl(chunkNo)&&counter>5) 
			break;
	}while(size==MAXBUF-offset);
}
int main(int argc, char** argv) {
	int fd,file;
	struct sockaddr_in addr;
	//Check for proper usage
	if(argc!=4) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}
	//Ignore SIGPIPE errors
	if(sethandler(SIG_IGN,SIGPIPE)) 
		ERR("Seting SIGPIPE:");
	//Set SIGALARM handler (for alarms)
	if(sethandler(sigalrm_handler,SIGALRM)) 
		ERR("Seting SIGALRM:");
	//Open file we'll operate on (task-specific)
	if((file=TEMP_FAILURE_RETRY(open(argv[3],O_RDONLY)))<0)
		ERR("open:");
	//Create a socket
	fd = make_socket();
	//Prepare an address
	addr=make_address(argv[1],argv[2]);
	//Do client work
	doClient(fd,addr,file);
	//Close socket file descriptor
	if(TEMP_FAILURE_RETRY(close(fd))<0)
		ERR("close");
	//Close file descriptor for file (task-specific)
	if(TEMP_FAILURE_RETRY(close(file))<0)
		ERR("close");
	return EXIT_SUCCESS;
}
