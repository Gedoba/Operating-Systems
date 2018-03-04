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
        exit(EXIT_SUCCESS);
        //kill(0, SIGINT);

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
    pid_t pid = getpid();
    srand(time(NULL)%pid);
    int random = rand()%21-10;
    char buff[MAX_BUFF];
    
    if(read(fds[n-1][0], buff, MAX_BUFF)<0)
        //ERR("read from fds");
        kill(0, SIGPIPE);

    printf("C%d: %s\n", pid, buff);
    int newNumber = atoi(buff);
        if(newNumber==0){
            printf("ending...\n");
            //kill(0, SIGINT);
            close(fds[n-1][0]); //close reading
            close(fds[n][1]); //close writing
        }
    newNumber+=random;
    sprintf(buff, "%d", newNumber);
    if(write(fds[n][1], buff, strlen(buff)+1)<0)
        //ERR("write to fds");
        kill(0, SIGPIPE);
    
}
void parent_work(int fds[3][2], int *isFirst) {
    pid_t pid = getpid();
    char buff[MAX_BUFF];
    int i = 1;
    sprintf(buff, "%d", i);
    if(*isFirst==1)
    {
        if(write(fds[0][1], buff, strlen(buff)+1)<0)
            //ERR("Write to fd0");
            kill(0, SIGPIPE);
        *isFirst = 0;
    }
    else if(*isFirst==0){
        if(read(fds[2][0], buff, MAX_BUFF)<0)
            //ERR("read from fds2");
            kill(0, SIGPIPE);
        printf("P%d: %s\n",pid, buff);
        int random = rand()%21-10;
        int newNumber = atoi(buff);
        if(newNumber==0){
            printf("ending...\n");
            //kill(0, SIGINT);
            close(fds[2][0]); //close reading
            close(fds[0][1]); //close writing
        }
            
        newNumber+=random;
        sprintf(buff, "%d", newNumber);
        if(write(fds[0][1], buff, strlen(buff)+1)<0)
            //ERR("Write to fd0");
            kill(0, SIGPIPE);
        
    }

    
}

void create_pipes(int fds[3][2]) {
    for(int i=0; i<3; i++){
        if(pipe(fds[i])==-1)
            ERR("pipe");
    }
}

void nsleep(long us)
{
    struct timespec wait;
    //printf("Will sleep for is %ld\n", diff); //This will take extra ~70 microseconds
    
    wait.tv_sec = us / (1000 * 1000);
    wait.tv_nsec = (us % (1000 * 1000)) * 1000;
    nanosleep(&wait, NULL);
}

void create_children(int fds[3][2]) {
    pid_t pid = getpid();
    srand(time(NULL)%pid);
    int n = 1;
    int *isFirstp;
    int isFirst = 1; // first call of parentWork
    isFirstp = &isFirst;
    printf("I've got %d more children to make, parent btw PID: %d\n", 3-n, getpid());
    int chld1=-1;
    int chld2=-1;
    chld1=fork();
    
    if(chld1==-1)
        ERR("Fork:");
    else if(chld1>0)
        chld2=fork();

    while(1){
        

            
        if(chld1==0 && chld2!=0){
            //printf("I'm a %d. child, PID: %d\n", 1, getpid());
            child_work(fds, 1);   
            //exit(EXIT_SUCCESS);
        }
        if(chld2==0 && chld1!=0){
            //printf("I'm a %d. child, PID: %d\n", 2, getpid());
            child_work(fds, 2);   
            //exit(EXIT_SUCCESS);
        }

        if(chld1>0 && chld2>0)
        {
            nsleep(10000);
            parent_work(fds, isFirstp);
          
        }
    }
}
void usage(char * name){
    fprintf(stderr,"USAGE: %s *randomString*\n",name);
    exit(EXIT_FAILURE);
}


int main(int argc, char** argv) {
    if(sethandler(sigPIPE_handler,SIGPIPE))
        ERR("Setting SIGPIPE handler");

    if(argc!=1)
        usage(argv[0]);

    int fds[3][2]={{0,0}, {0,0}, {0,0}}; //used to store two ends of pipes 1-3

    create_pipes(fds);
    create_children(fds);

    return EXIT_SUCCESS;
}