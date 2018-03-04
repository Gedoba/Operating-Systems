#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#define ERR(source) (fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     perror(source),kill(0,SIGKILL),\
                          exit(EXIT_FAILURE))
                                            
void usage(char *name){
	fprintf(stderr,"USAGE: %s 0<n\n",name);
	exit(EXIT_FAILURE);
}
void create_children(int n);
void child_work();

int main(int argc, char** argv) {
    int n;
    if(argc<2){
        usage(argv[0]);
    }
    n = atoi(argv[1]);
    if(n<=0){
        usage(argv[0]);
    }
    create_children(n);

    while(n>0){
        sleep(3);
        pid_t pid;
        while(1){
            pid = waitpid(0, NULL, WNOHANG);
            if(pid>0)
                n--;
            if(0==pid)
                break;
            if(0>pid){
                if(ECHILD==errno)
                    break;
                ERR("waitpid: ");
            }
            printf("PARENT: %d processes remain\n", n);
        }
    }
	return EXIT_SUCCESS;
}

void create_children(int n){
    pid_t pid;
    printf("Parent's pid: %d\n", getpid());
    //int counter = 0;
    for(; n>0; n--){
        //counter++;
        if((pid=fork())<0)
        //pid is a parent process, but child coundn't be created
            ERR("Fork: ");
        if(pid==0){
        //pid is a child process
            printf("I'm a child, my pid: %d, ppid: %d\n", getpid(), getppid());
            child_work();
            exit(EXIT_SUCCESS);
        }
    }
   // printf("Counter: %d\n", counter);
}

void child_work(){
    srand(time(NULL)*getpid());
    int t = 5 + rand()%6;
    sleep(t);
    printf("PROCESS with pid %d terminates\n",getpid());
}