#define _GNU_SOURCE 
#include <stdio.h>
//Semaphore alarm, server side
//Commented
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
#include <pthread.h>
#include <semaphore.h>

//Error macro
#define ERR(source) (perror(source),\
		     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
		     exit(EXIT_FAILURE))

//Semaphore's initial value
#define FS_NUM 10 

//Signal handling section start
volatile sig_atomic_t do_work=1 ;

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
//Signal handling section end

//Agnostic socket creation - may be UDP over IP, may be TCP over local streams
int make_socket(int domain, int type){
	int sock;
	sock = socket(domain,type,0);
	if(sock < 0)
        ERR("socket");
	return sock;
}

//Binding an internet socket
int bind_inet_socket(uint16_t port,int type){
	struct sockaddr_in addr;
	int socketfd,t=1;
	socketfd = make_socket(PF_INET,type);
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY); //Work on all devices connected to system
	if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR,&t, sizeof(t))) //Reuse addresses
        ERR("setsockopt");
	if(bind(socketfd,(struct sockaddr*) &addr,sizeof(addr)) < 0) //Listen to requests
        ERR("bind");
	return socketfd;
}

//Usage
void usage(char * name){
	fprintf(stderr,"USAGE: %s  port\n",name);
}

//Structure to hold data related to client (socket, time to wait, address to send to, semaphore)
struct arguments {
	int fd;
	int16_t time;
	struct sockaddr_in addr;
	sem_t* semaphore;
};

//
void* communicateDgram(void *arg){
	struct arguments  *args= (struct arguments*) arg;
	int tt;
    //Communicate you will sleep
	fprintf(stderr,"Will sleep for %d\n",ntohs(args->time));
    //Sleep for given amount of time - for loop meant to ensure signals don't break the sleep
	for (tt = ntohs(args->time); tt > 0; tt = sleep(tt));
    //Try to send data back to client - errno part meant to ensure lack of client doesn't break app
    //Sent back: time that has elapsed (as ensured by for loop)
	if(TEMP_FAILURE_RETRY(sendto(args->fd,(char *)(&(args->time)),sizeof(int16_t),0,&(args->addr),sizeof(args->addr)))<0&&errno!=EPIPE)
        ERR("sendto");
	if (sem_post(args->semaphore) == -1) //Post == UNLOCK
        ERR("sem_post");
    //Args need to be freed - no idea why
	free(args);
	return NULL;
}

//Server's work function
void doServer(int fd){
	int16_t time;
	int16_t deny=-1;
	deny=htons(deny);
	pthread_t thread;
	struct sockaddr_in addr;
	struct arguments *args;
	socklen_t size=sizeof(struct sockaddr_in);
	sem_t semaphore;
    //Initialize the semaphore
	if (sem_init(&semaphore, 0, FS_NUM) != 0)
		ERR("sem_init");
    //Work until SIGINT caught
	while(do_work){
        //Try to get something from socket
        if(recvfrom(fd,(char *)&time,sizeof(int16_t),0,&addr,&size)<0) {
            if(errno==EINTR) //If receive broken by SIGINT: just try again
                continue ;
			ERR("recvfrom:");
		}
        //trywait means UNLOCK if POSSIBLE (don't wait!)
        //Here we go into if block if we CAN'T block for some reason
		if (TEMP_FAILURE_RETRY(sem_trywait(&semaphore)) == -1) {
			switch(errno){
            //EAGAIN means semaphore is at 0: can't lock
			case EAGAIN:
                    //Hence we send a message to client it can't be served
                    if(TEMP_FAILURE_RETRY(sendto(fd,(char *)&deny,sizeof(int16_t),0,&addr,sizeof(addr)))<0&&errno!=EPIPE)
                        ERR("sendto");
            //In case of SIGINT: just try again with another connection
			case EINTR:
                    continue;
			}
            //Otherwise crash the app
			ERR("sem_wait");
		}
        //Allocate space for struct arguments that'll be passed to pthread
		if((args=(struct arguments*)malloc(sizeof(struct arguments)))==NULL) ERR("malloc:");
		args->fd=fd;
		args->time=time;
		args->addr=addr;
		args->semaphore=&semaphore;
        //Create a pthread running communicateDgram with created struct args
		if (pthread_create(&thread, NULL,communicateDgram, (void *)args) != 0)
            ERR("pthread_create");
        //Make it clear you don't need pthread's data after it finishes
		if (pthread_detach(thread) != 0)
            ERR("pthread_detach");
	}
}

//Main function
int main(int argc, char** argv) {
	int fd;
	if(argc!=2) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}
    
	if(sethandler(SIG_IGN,SIGPIPE)) //Ignore SIGPIPE (it's not fatal)
        ERR("Seting SIGPIPE:");
	if(sethandler(sigint_handler,SIGINT)) //Handle SIGINT (to allow clean exit)
        ERR("Seting SIGINT:");
    //Bind the socket with provided port and UDP as mode of communication
	fd=bind_inet_socket(atoi(argv[1]),SOCK_DGRAM);
    //Do work
	doServer(fd);
    //After work: close socket
	if(TEMP_FAILURE_RETRY(close(fd))<0)
        ERR("close");
	fprintf(stderr,"Server has terminated.\n");
	return EXIT_SUCCESS;
}

