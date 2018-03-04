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

void removeChar(char* inputString){
    srand(getpid());
    int r;
    if(strlen(inputString)==0)
        r=0;
    else
        r = rand()%strlen(inputString);
    memmove(&inputString[r], &inputString[r + 1], strlen(inputString) - r);
}

int child_work(int *fd1, int *fd2) {
    pid_t pid = getpid();
    char buff[MAX_BUFF];
    // Close writing end of first pipe
    //close(fd1[1]);
    // Close reading of second pipe
    //close(fd2[0]);
    read(fd1[0], buff, MAX_BUFF);
    // Close reading of first pipe
    //close(fd1[0]);
    printf("My buff (PID: %d): %s\n", pid, buff);
    removeChar(buff);
    write(fd2[1], buff, strlen(buff)+1);
    // Close writing of second pipe
    //close(fd2[1]);
    
    return strlen(buff);
}
int parent_work(char* inputString, int* fd1, int* fd2) {
    pid_t pid = getpid();
    //char buff[MAX_BUFF];
    // Close reading end of first pipe
    //close(fd1[0]);
    write(fd1[1], inputString, strlen(inputString)+1);
    // Close writing of first pipe
    //close(fd1[1]);
    //wait(NULL); //wait for child to finish
    // Close writing end of second pipe
    //close(fd2[1]);        
    read(fd2[0], inputString, MAX_BUFF);
    printf("My buff (PID: %d): %s\n", pid, inputString);
    removeChar(inputString);
    write(fd1[1], inputString, strlen(inputString)+1);
    //close reading at second pipe
    //close(fd2[0]);
    return strlen(inputString);
}
void create_pipes(int *fd1, int *fd2) {
    if(pipe(fd1)==-1)
        ERR("pipe");
    if(pipe(fd2)==-1)
        ERR("pipe");
}
void usage(char * name){
    fprintf(stderr,"USAGE: %s *randomString*\n",name);
    exit(EXIT_FAILURE);
}


int main(int argc, char** argv) {
    int fd1[2];  // Used to store two ends of first pipe
    int fd2[2];  // Used to store two ends of second pipe

    if(sethandler(sigPIPE_handler,SIGPIPE))
        ERR("Setting SIGPIPE handler");
    if(sethandler(sigINT_handler,SIGINT))
        ERR("Setting SIGINT handler");
    

    char buff[MAX_BUFF];
    
    if(argc!=2)
        usage(argv[0]);

    strcpy(buff, argv[1]);

    create_pipes(fd1, fd2);

    pid_t p;
    int ret=1;
    p=fork();

    if(p<0)
        ERR("fork");
   
    while(last_signal){
        //parent
        if(p>0)
        {
            ret=parent_work(buff, fd1, fd2);
            //wait(NULL);
            sleep(1);
            if(ret==0)
                break;                
        }
        //child
        else if(p==0)
        {
            ret=child_work(fd1, fd2);
            sleep(1);
            if(ret==0)
                break;
        }
    }

    //closing could've been done a bit more secure (earlier)
    close(fd1[0]);
    close(fd1[1]);
    close(fd2[1]); 
    close(fd2[0]);

    return EXIT_SUCCESS;
}