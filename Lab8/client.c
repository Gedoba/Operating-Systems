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
#define CHUNKSIZE 10
volatile sig_atomic_t last_signal=0 ;


int make_socket(void){
        int sock;
        sock = socket(PF_INET,SOCK_DGRAM,0);
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
int main(int argc, char** argv) {
    int fd;
    struct sockaddr_in addr;
    fd = make_socket();
    char* address = "localhost";
    char* port = "2000";
    addr=make_address(address,port);
    char buf[CHUNKSIZE];
    if(TEMP_FAILURE_RETRY(sendto(fd,(char *)argv[1],sizeof(char*),0,&addr,sizeof(addr)))<0)
        ERR("sendto:");
    while(recv(fd,(char *)&buf,CHUNKSIZE,0)<0){
        if(EINTR!=errno)ERR("recv:");
    }
    printf("Connected to server\n");
    if(TEMP_FAILURE_RETRY(close(fd))<0)ERR("close");
    return EXIT_SUCCESS;
}