#define _GNU_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <signal.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
#define ERR(source) (perror(source),\
                     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     exit(EXIT_FAILURE))
#define CHUNKSIZE 1
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
        return socketfd;
}
struct arguments {
        int fd;
        struct sockaddr_in addr;
        char* sym;
};
void* communicateDgram(void *arg){
        struct arguments  *args= (struct arguments*) arg;
        char* buf = "OK";
        printf("Client <%s> is accepted\n",args->sym);
        if(TEMP_FAILURE_RETRY(sendto(args->fd,(char *)buf,sizeof(buf),0,&(args->addr),sizeof(args->addr)))<0) ERR("sendto");
        free(args);
        return NULL;
}
void doServer(int fd){
        pthread_t thread;
        struct sockaddr_in addr;
        struct arguments *args;
        char buf[1];
        socklen_t size=sizeof(struct sockaddr_in);
        while(do_work){
                if(recvfrom(fd,(char *)buf,sizeof(char *),0,&addr,&size)<0) {                      
                        if(errno==EINTR) continue ;
                        ERR("recvfrom:");
                }
                if((args=(struct arguments*)malloc(sizeof(struct arguments)))==NULL) ERR("malloc:");
                args->fd=fd;
                args->addr=addr;
                args->sym=buf;
                if (pthread_create(&thread, NULL,communicateDgram, (void *)args) != 0) ERR("pthread_create");
                if (pthread_detach(thread) != 0) ERR("pthread_detach");
        }
}

int main(int argc, char** argv) {
    int fd;
    if(sethandler(sigint_handler,SIGINT)) ERR("Seting SIGINT:");
    fd=bind_inet_socket(atoi(argv[1]),SOCK_DGRAM);
    doServer(fd);
    if(TEMP_FAILURE_RETRY(close(fd))<0)ERR("close");
    fprintf(stderr,"Server has terminated.\n");
    return EXIT_SUCCESS;
}