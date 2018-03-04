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
                     perror(source),kill(0, SIGKILL),\
                          exit(EXIT_FAILURE))

                          
volatile sig_atomic_t sig_count = 0;

void sig_handler(int sig) {
	sig_count++;;
}

void sethandler(void (*f)(int), int sigNo) {
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
    if (-1==sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

void child_work() {
	struct timespec t = {0, 180000000};
	sethandler(SIG_DFL,SIGUSR1);
	while(1){ 
		nanosleep(&t,NULL);
        if(kill(getppid(),SIGUSR1))
            ERR("kill");
	}
}

void parent_work(){
    sethandler(sig_handler, SIGUSR1);
    printf("Im a parent %d\n", getpid());
    printf("Signals RX:%d\n",sig_count);


}
                         
int main(int argc, char** argv) {
	int n;
    if(argc!=2)
        ERR("usage");
	sethandler(sig_handler,SIGUSR1);
	pid_t pid;
    if((pid=fork())<0)
        ERR("fork");
    if(0==pid)
        child_work();
	else {
		parent_work();
		while(wait(NULL)>0);
	}
	return EXIT_SUCCESS;
}