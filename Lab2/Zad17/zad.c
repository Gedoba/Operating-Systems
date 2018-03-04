#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#define ERR(source) (fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     perror(source),kill(0,SIGKILL),\
                          exit(EXIT_FAILURE))
                                            
volatile sig_atomic_t sig_count = 0;
volatile sig_atomic_t last_signal = 0;
int noargs = 0;
int function( int );
void usage(char *name){
        fprintf(stderr,"USAGE: %s 0<n\n",name);
        exit(EXIT_FAILURE);
    }
    
void sig_handler(int sig)
{
    //printf("[%d] received signal %d\n", getpid(), sig);
    if(sig==SIGUSR1){
        printf("[%d] received SIGUSR1, count: %d\n",getpid(), sig_count*noargs);
        sig_count++;
    }
      
}

void sigalarm_handler(int sig){
    last_signal = sig;
}
    
void sethandler( void (*f)(int), int sigNo) {
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
    if (-1==sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

void child_work(int n){ // n is parameter from stdin
    srand(time(NULL)*getpid());
    int s = rand()%91+10;
    s*=1024; // ->KB
    int out;
    char *buf=malloc(s);
    	struct timespec t = {0, 1000};
    if(!buf)
        ERR("malloc");
    char name[10];
    memset(buf, n, s);
    sprintf(name, "%d.txt", getpid());

    if((out=open(name,O_WRONLY|O_CREAT|O_TRUNC|O_APPEND,0777))<0)
        ERR("open");
    if((write(out,buf,s))<0)
        ERR("write");
    if(close(out))
        ERR("close");
    free(buf);
    printf("My pid %d,ppid %d,gid: %d n is %d, and s is %dB=%dKB\n",getpid(), getppid(),getgid(), n, s, s/1024);
    
    last_signal=0;

        while(last_signal!=SIGUSR1)
        {
            if(last_signal==SIGALRM)
			{
                break;
			}
            nanosleep(&t,NULL);
		}
}



void parent_work(){
    sethandler(sigalarm_handler,SIGALRM);
    sethandler(SIG_IGN,SIGUSR1);
    struct timespec t = {0, 10*1000*1000};
    alarm(1);
    while(last_signal!=SIGALRM){
        nanosleep(&t, NULL);
        if(kill(0, SIGUSR1)<0)
            ERR("kill");
        printf("sent\n");
        }
    printf("DED\n");
}
void create_children(int n){
    pid_t s;
    if((s=fork())<0)
        ERR("fork");
    if(!s){
        sethandler(sig_handler,SIGUSR1);
        child_work(n);
        exit(EXIT_SUCCESS);
    }
}

void sigchld_handler(int sig) {
	pid_t pid;
	for(;;){
		pid=waitpid(0, NULL, WNOHANG);
		if(pid==0) return;
		if(pid<=0) {
			if(errno==ECHILD) return;
			ERR("waitpid");
		}
	}
}
   
int main(int argc, char** argv){
    noargs = argc; //how to do that without global variable
    if(argc<2)
        usage(argv[0]);
    for(int i=1; i<argc; i++){
        int n = atoi(argv[i]);
        if(n>9 || n<0){
            usage(argv[0]);
        }
        else{
            create_children(n);
        }
    }
    parent_work();
    //while(wait(NULL)>0);
    return EXIT_SUCCESS;
}