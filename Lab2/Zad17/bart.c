#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#define ERR(source) (fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     perror(source),kill(0,SIGKILL),\
		     		     exit(EXIT_FAILURE))

volatile sig_atomic_t last_signal = 0;
volatile sig_atomic_t counter = 0;

void sethandler( void (*f)(int), int sigNo) {
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (-1==sigaction(sigNo, &act, NULL)) ERR("sigaction");
}

void usage () {
	printf("failure");
	exit(EXIT_FAILURE);
}

void sig_handler(int sig) {
	//printf("[%d] received signal %d\n", getpid(), sig);
	last_signal = sig;
	if(sig==SIGUSR1) counter++;
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

void child_work() {
	srand(time(NULL)*getpid());
	int s=100+rand()%101;
	printf("My random time is %d\n",s);
	struct timespec t1 = {0, s*1000*1000};
	int amt=30;
	while(1) {
		nanosleep(&t1,NULL);
		if (last_signal!=SIGUSR2 && amt>0) {
			kill(getppid(), SIGUSR1);
			fprintf(stderr,"*");
			amt--;
		} else {
			break;
		}
	}
	fprintf(stderr,"HE DED\n");
}

void create_children(int amount) {
	while (amount-->0) {
		switch (fork()) {
			case 0: 
				child_work();
				exit(EXIT_SUCCESS);
			case -1:perror("Fork:");
				exit(EXIT_FAILURE);
		}
	}
}

void parent_work() {
	struct timespec t1={0, 1};
	while(1) {
		last_signal=0;
		while(last_signal!=SIGUSR1); 
			nanosleep(&t1,NULL);
		TEMP_FAILURE_RETRY(fprintf(stderr,"%d\n",counter));
		if(counter==100) break;
	}
	kill(0, SIGUSR2);
}

int main(int argc, char** argv) {
	int amount;
	if(argc!=2) usage();
	amount = atoi(argv[1]);
	if (amount<=0)  usage(); 
	sethandler(sigchld_handler,SIGCHLD);
	sethandler(sig_handler,SIGUSR1);
	sethandler(sig_handler,SIGUSR2);
	create_children(amount);
	parent_work();
	while(wait(NULL)>0);
	return EXIT_SUCCESS;
}