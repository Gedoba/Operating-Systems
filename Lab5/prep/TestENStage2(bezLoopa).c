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
#define MAX_BUFF 20
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

void removeChar(char* inputString){
    srand(getpid());
    int r = rand()%strlen(inputString)-1;
    memmove(&inputString[r], &inputString[r + 1], strlen(inputString) - r);
}

void child_work(int *fd1, int *fd2) {
    pid_t pid = getpid();
    char buff[MAX_BUFF];
    printf("I'm a child process, PID: %d\n", pid);

    if(0==close(fd1[1]))  // Close writing end of first pipe
        printf("PID: %d, close(), descriptor number: %d\n", pid, 1);
    read(fd1[0], buff, MAX_BUFF);

    printf("My buff (child): %s\n", buff);

    //close both reading ends
    if(0==close(fd1[0]))
        printf("PID: %d, close(), descriptor number: %d\n", pid, 0);
    if(0==close(fd2[0]))
        printf("PID: %d, close(), descriptor number: %d\n", pid, 0);
    removeChar(buff);

    write(fd2[1], buff, strlen(buff)+1);
    //close second writing end
    if(0==close(fd2[1]))
        printf("PID: %d, close(), descriptor number: %d\n", pid, 1);

}
void parent_work(const char* inputString, int* fd1, int* fd2) {
    pid_t pid = getpid();
    char buff[MAX_BUFF];
    printf("I'm a parent process, PID: %d\n", pid);
    printf("input: %s\n", inputString);


    // Close reading end of first pipe
    if(0==close(fd1[0]))
        printf("PID: %d, close(), descriptor number: %d\n", pid, 0);
    write(fd1[1], inputString, strlen(inputString)+1);
    // Close writing of first pipe
    if(0==close(fd1[1]))
        printf("PID: %d, close(), descriptor number: %d\n", pid, 1);
    wait(NULL); //wait for child to finish

    // Close writing end of second pipe
    if(0==close(fd2[1]))
        printf("PID: %d, close(), descriptor number: %d\n", pid, 1);
        
    read(fd2[0], buff, MAX_BUFF);
    printf("My buff (parent): %s\n", buff);
    //close reading at second pipe
    if(0==close(fd2[0]))
        printf("PID: %d, close(), descriptor number: %d\n", pid, 0);
    


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
}
void usage(char * name){
        fprintf(stderr,"USAGE: %s *randomString*\n",name);
        exit(EXIT_FAILURE);
}


int main(int argc, char** argv) {
    int fd1[2];  // Used to store two ends of first pipe
    int fd2[2];  // Used to store two ends of second pipe

    if(argc!=2)
        usage(argv[0]);

    create_pipes(fd1, fd2);

    pid_t p;
    p=fork();

    if(p<0)
        ERR("fork");
    //parent
    else if(p>0)
    {
        parent_work(argv[1], fd1, fd2);
    }
    //child
    else
    {
        child_work(fd1, fd2);
    }

    return EXIT_SUCCESS;
}