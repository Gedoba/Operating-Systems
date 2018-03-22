#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <mqueue.h>

#define LIFE_SPAN 10
#define MAX_NUM 10

#define ERR(source) (fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     perror(source),kill(0,SIGKILL),\
		     		     exit(EXIT_FAILURE))

volatile sig_atomic_t children_left = 0;

void sethandler( void (*f)(int, siginfo_t*, void*), int sigNo) {
    
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_sigaction = f;
	act.sa_flags=SA_SIGINFO;
	if (-1==sigaction(sigNo, &act, NULL))
		ERR("sigaction");
}

void sigchld_handler(int sig, siginfo_t *s, void *p) {
	pid_t pid;
	for(;;){
		pid=waitpid(0, NULL, WNOHANG);
		if(pid==0)
            return;
		if(pid<=0) {
			if(errno==ECHILD)
                return;
			ERR("waitpid");
		}
		children_left--;
	}
}

void child_work() {

}


void parent_work() {

}

void create_children(int n, mqd_t pin, mqd_t pout) {
	while (n-->0) {
		switch (fork()) {
			case 0: child_work(n,pin,pout);
				exit(EXIT_SUCCESS);
			case -1:perror("Fork:");
				exit(EXIT_FAILURE);
		}
		children_left++;
	}

}
void usage(void){
	fprintf(stderr,"USAGE: signals n k p l\n");
	fprintf(stderr,"100 > n > 0 - number of children\n");
	exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {
	int n; 
	if(argc!=2) usage();
	n = atoi(argv[1]);
	if (n<=0||n>=100)
        usage(); 

	return EXIT_SUCCESS;
}