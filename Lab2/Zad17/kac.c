#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#define ERR(source) (fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     perror(source),kill(0,SIGKILL),\
                                     exit(EXIT_FAILURE))

volatile sig_atomic_t last_signal = 0;
volatile sig_atomic_t sig_count = 0;
void usage(char *name){
        fprintf(stderr,"USAGE: %s 0<n\n",name);
}
    
void sig_handler(int sig)
{
    sig_count++;
}

void sigalarm_handler(int sig){
    last_signal = sig;
}
    
void sethandler( void (*f)(int), int sigNo) {
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (-1==sigaction(sigNo, &act, NULL)) ERR("sigaction");
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

void parent_work(){
    struct timespec t = {0, 10*1000000};
    sethandler(sigalarm_handler,SIGALRM);
    alarm(1);
    while(last_signal != SIGALRM){
        nanosleep(&t,NULL);
        if (kill(0, SIGUSR1)<0)ERR("kill");
    }

}
ssize_t bulk_write(int fd, char* buf, size_t count)
{
    ssize_t c;
    ssize_t len=0;
    do
    {
        c=TEMP_FAILURE_RETRY(write(fd,buf,count));
        if(c<0) return c;
        buf +=c;
        len +=c;
        count -=c;

    }while(count>0);
    return len;
}


void child_work(int n) {
        struct timespec T = {1, 0};
        struct timespec t = {0,0};
        srand(time(NULL)*getpid());     
        int s=1+rand()%(10-1);
        s = s*10*1024;
        int out;
        char *buf=malloc(s);
        char name[10];
        memset(buf,n,s);
        sprintf(name,"%d.txt",getpid());

        for(;T.tv_sec!=0 || T.tv_nsec!=0;){
            nanosleep(&T,&t);
            T.tv_sec = t.tv_sec;
            T.tv_nsec = t.tv_nsec;
            t.tv_sec = 0;
            t.tv_nsec = 0;
        }

        if((out=TEMP_FAILURE_RETRY(open(name,O_WRONLY|O_CREAT|O_TRUNC|O_APPEND,0777)))<0)ERR("open");
        for(int i =0;i<sig_count;i++){
            if(bulk_write(out,buf,s)<0)ERR("write");
        }
        if(close(out))ERR("close");
        free(buf);
        

        printf("PROCESS with pid %d got numbers: s: %d n: %d sig_count: %d\n",getpid(),s,n,sig_count);
}

void create_children(int n) {
        pid_t s;
                if((s=fork())<0) ERR("Fork:");
                if(!s) {
                        sethandler(sig_handler,SIGUSR1);
                        child_work(n);
                        exit(EXIT_SUCCESS);
                }
}


int main(int argc, char** argv) {
        if(argc<2)  usage(argv[0]);
        sethandler(SIG_IGN,SIGUSR1);
        sethandler(sigchld_handler, SIGCHLD);
        for(int i = 1;i<argc;i++){
            create_children(atoi(argv[i]));
        }
        parent_work();
        while(wait(NULL)>0);
        return EXIT_SUCCESS;
}