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
void child_work() {
        pid_t pid = getpid();
        printf("I'm a child process, PID: %d\n", pid);
}
void parent_work() {
        pid_t pid = getpid();
        printf("I'm a parent process, PID: %d\n", pid);
}
void create_pipes(int *fd1, int *fd2) {
    pid_t pid = getpid();
    if(pipe(fd1)==-1)
        ERR("pipe");
    else
        printf("PID: %d, pipe()\n", pid);
    if(pipe(fd2)==-1)
        ERR("pipe");
    else
        printf("PID: %d, pipe()\n", pid);

    if(!close(fd1[1]))
        printf("PID: %d, close(), descriptor number: %d\n", pid, 1);
    if(!close(fd1[0]))
        printf("PID: %d, close(), descriptor number: %d\n", pid, 0);
    if(!close(fd2[1]))
        printf("PID: %d, close(), descriptor number: %d\n", pid, 1);
    if(!close(fd2[0]))
        printf("PID: %d, close(), descriptor number: %d\n", pid, 0);
}
void usage(char * name){
        fprintf(stderr,"USAGE: %s n\n",name);
        fprintf(stderr,"0<n<=10 - number of children\n");
        exit(EXIT_FAILURE);
}
int main(int argc, char** argv) {
    int fd1[2];  // Used to store two ends of first pipe
    int fd2[2];  // Used to store two ends of second pipe

    create_pipes(fd1, fd2);

    pid_t p;
    p=fork();

    if(p<0)
        ERR("fork");
    //parent
    else if(p>0)
    {
        parent_work();
    }
    //child
    else
    {
        child_work();
    }

    return EXIT_SUCCESS;
}