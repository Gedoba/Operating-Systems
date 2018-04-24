//Semaphore alarm, client side
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
#include <netinet/in.h>
#include <signal.h>
#include <netdb.h>

//Error macro
#define ERR(source) (perror(source),\
		     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
		     exit(EXIT_FAILURE))
//German error macro (not really, just error handler for gethostbyname)
#define HERR(source) (fprintf(stderr,"%s(%d) at %s:%d\n",source,h_errno,__FILE__,__LINE__),\
		     exit(EXIT_FAILURE))

#define TIMEOUT 15

//Signal handling section start
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
//Signal handling section end

//Socket creation - it's UDP on IP
int make_socket(void){
	int sock;
    //INET - IP, DGRAM - UDP
	sock = socket(PF_INET,SOCK_DGRAM,0);
	if(sock < 0)
        ERR("socket");
	return sock;
}

//Make address
struct sockaddr_in make_address(char *address, uint16_t port){
	struct sockaddr_in addr;
	struct hostent *hostinfo;
	addr.sin_family = AF_INET;
	addr.sin_port = htons (port);
    //Here we try to get the server from provided address
	hostinfo = gethostbyname(address);
	if(hostinfo == NULL)
        HERR("gethostbyname"); //HERR-Fehlerbehandlung benutzt
	addr.sin_addr = *(struct in_addr*) hostinfo->h_addr;
	return addr;
}

//Usage, duh
void usage(char * name){
	fprintf(stderr,"USAGE: %s domain port time \n",name);
}

//Main function
int main(int argc, char** argv) {
	int fd;
	struct sockaddr_in addr;
	int16_t time;
	int16_t deny=-1;
	deny=htons(deny);
	if(argc!=4) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}
	if(sethandler(SIG_IGN,SIGPIPE)) //Ignore SIGPIPE
        ERR("Seting SIGPIPE:");
	if(sethandler(sigalrm_handler,SIGALRM)) //Keep track of SIGALARM
        ERR("Seting SIGALRM:");
    //Create a socket
	fd = make_socket();
    //Get server address
	addr=make_address(argv[1],atoi(argv[2]));
    //Get time for alarm into network order
	time=htons(atoi(argv[3]));
	/*
	 * Broken PIPE is treated as critical error here
	 */
    //Remember lab 7? This comment occurred again, again not on website, but in files. Ugh.
    
    //Try to send alarm time to server
	if(TEMP_FAILURE_RETRY(sendto(fd,(char *)&time,sizeof(int16_t),0,&addr,sizeof(addr)))<0)
		ERR("sendto:");
    //Set up timeout - last_signal will be set to SIGALRM when TIMEOUT elapses
	alarm(TIMEOUT);
    //Try to get a message back from server - loop stops either after TIMEOUT elapses
    //or when you get the message back, meaning alarm worked
	while(recv(fd,(char *)&time,sizeof(int16_t),0)<0){
		if(EINTR!=errno)
            ERR("recv:");
		if(SIGALRM==last_signal)
            break;
	}
	
    //Handle all possible cases now: timeout, denial or things working correctly
	if(last_signal==SIGALRM)
        printf("Timeout\n");
	else if (time==deny)
        printf("Service denied\n");
	else
        printf("Time has expired\n");

    //Close the sockets
	if(TEMP_FAILURE_RETRY(close(fd))<0)
        ERR("close");
	return EXIT_SUCCESS;
}

