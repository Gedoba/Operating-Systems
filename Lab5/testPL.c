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
volatile sig_atomic_t last_signal = -1;
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


void sigINT_handler(int sig) {
        last_signal = 0;
}
void sigPIPE_handler(int sig) {
        last_signal = sig;
        printf("Broken pipe\nExiting...\n");

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

void child_work(int fds[3][2], int n) {
    printf("child working no. = %d...\n", n);
    pid_t pid = getpid();
    srand(pid);
    int random = rand()%100;
    char buff[MAX_BUFF];

    sprintf(buff, "%d", random);
    if(write(fds[n][1], buff, strlen(buff)+1)<0)
        ERR("write to fds");
    if(read(fds[n-1][0], buff, MAX_BUFF)<0)
        ERR("read from fds");
    printf("My buffChild (PID: %d): %s\n", pid, buff);
}
void parent_work(int fds[3][2]) {
    printf("parent working...\n");
    pid_t pid = getpid();
    srand(pid);
    int random = rand()%100;
    char buff[MAX_BUFF];
    
    sprintf(buff, "%d", random);
    printf("parents buff:%s\n", buff);
    if(read(fds[2][0], buff, MAX_BUFF)<0)
        ERR("read from fds2");
    if(write(fds[0][1], buff, strlen(buff)+1)<0)
        ERR("Write to fd0");
    
    printf("My buffParent (PID: %d): %s\n", pid, buff);
   

}
void create_pipes(int fds[3][2]) {
    int n = 1;


    pid_t child_pid, wpid;
    int status = 0;

    if(pipe(fds[0])==-1)
        ERR("pipe");
    else
        printf("Made pipe0\n");
   
    while(n!=3){
        //wait(NULL);
        printf("I've got %d more children to make, parent btw PID: %d\n", 3-n, getpid());
        int p=fork();
        switch(p){
            default:
                parent_work(fds);
            case 0: 
                printf("I'm a %d. child, PID: %d\n", n, getpid());
                if(pipe(fds[n])==-1)
                    ERR("pipe");
                else{
                    sleep(1);
                    printf("Made pipe%d\n", n);
                    child_work(fds, n);
                    //if(!close(fds[3-n][0])&&!close(fds[3-n][1]))
                    //    printf("closed both descriptors of pipe%d\n", 3-n);
                }
                
                    
                exit(EXIT_SUCCESS);
            case -1: ERR("Fork:");

        }
        n++;
    }
    //while ((wpid = wait(&status)) > 0);
}
void usage(char * name){
    fprintf(stderr,"USAGE: %s *randomString*\n",name);
    exit(EXIT_FAILURE);
}


int main(int argc, char** argv) {
    // int fd0[2]={0,0};  // Used to store two ends of first pipe
    // int fd1[2]={0,0};  // Used to store two ends of second pipe
    // int fd2[2]={0,0};  // Used to store two ends of third pipe
    int fds[3][2]={{0,0}, {0,0}, {0,0}}; //used to store two ends of pipes 1-3
    
    if(argc!=1)
        usage(argv[0]);

    create_pipes(fds);

    return EXIT_SUCCESS;
}