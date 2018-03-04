#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#define ERR(source) (fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     perror(source),kill(0,SIGKILL),\
                     exit(EXIT_FAILURE))

//MAX_BUFF must be in one byte range
#define MAX_BUFF 200
volatile sig_atomic_t last_signal = 0;
int sethandler( void (*f)(int), int sigNo) {
        struct sigaction act;
        memset(&act, 0, sizeof(struct sigaction));
        act.sa_handler = f;
        if (-1==sigaction(sigNo, &act, NULL))
            return -1;
        return 0;
}
void sig_handler(int sig) {
        last_signal = sig;
}
void sig_killme(int sig) {
        if(rand()%5==0)
            exit(EXIT_SUCCESS);
}
void sigchld_handler(int sig) {
        pid_t pid;
        for(;;){
                pid=waitpid(0, NULL, WNOHANG);
                if(0==pid)
                    return;
                if(0>=pid) {
                        if(ECHILD==errno)
                            return;
                        ERR("waitpid:");
                }
        }
}
void child_work(int fd, int R) {
        
}
void parent_work() {
      
}
void create_children_and_pipes() {
       
}
void usage(char * name){
        fprintf(stderr,"USAGE: %s n\n",name);
        fprintf(stderr,"0<n<=10 - number of children\n");
        exit(EXIT_FAILURE);
}
int main(int argc, char** argv) {
        
        return EXIT_SUCCESS;
}