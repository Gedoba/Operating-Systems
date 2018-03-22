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


void mq_handler(int sig, siginfo_t *info, void *p) {
	mqd_t *pin;
	uint8_t ni;
	unsigned msg_prio;

	pin = (mqd_t *)info->si_value.sival_ptr;

	static struct sigevent not;
	not.sigev_notify=SIGEV_SIGNAL;
	not.sigev_signo=SIGRTMIN;
	not.sigev_value.sival_ptr=pin;

	if(mq_notify(*pin, &not)<0)
		ERR("mq_notify");

	for(;;)
    {
		if(mq_receive(*pin,(char*)&ni,1,&msg_prio)<1)
        {
			if(errno==EAGAIN)
                break;
			else
                ERR("mq_receive");
		}
		if(0==msg_prio)
            printf("MQ: got timeout from %d.\n",ni+1);
		else //1 -> bingo
            printf("MQ:%d is a bingo number!\n",ni);
	}

}


void child_work(mqd_t pout, mqd_t pin, int n) {
    int life;
    srand(getpid());
    life = rand()%LIFE_SPAN+1; 
    uint8_t received, bingo;
    bingo = (uint8_t)rand() % MAX_NUM;
    while(life--){
        if(TEMP_FAILURE_RETRY(mq_receive(pout,(char *)&received,1, 0))==-1)
            ERR("mq receive");
        printf("[%d] received: %d\n", getpid(), received);
        if(bingo==received)
        {
            if(TEMP_FAILURE_RETRY(mq_send(pin, (const char *)&bingo, 1, 1))) //prio 1 means bingo
                ERR("mq_send");
            printf("BINGO, %d. terminates...\n", n+1);
            return;
        }
            
    }
    if(TEMP_FAILURE_RETRY(mq_send(pin, (const char*)&n, 1, 0)))
        ERR("mq_send");
    printf("[%d] terminates\n", getpid());
}


void create_children(int n, mqd_t pout, mqd_t pin) {
    while(n-->0){
        switch(fork()){
            case 0: child_work(pout, pin, n);
                exit(EXIT_SUCCESS);
            case -1: perror("fork");
                exit(EXIT_FAILURE);
        }
        children_left++;
    }
}

void parent_work(mqd_t pout) {
    srand(getpid());
    
    uint8_t rNo;
    while(children_left){
        rNo = (uint8_t)rand()%MAX_NUM;
        if(TEMP_FAILURE_RETRY(mq_send(pout,(const char*)&rNo,1, 0)))
            ERR("mq send");
        sleep(1);
        //printf("%d sent\n", rNo);
    }
    printf("[PARENT] Terminates \n");
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
    mqd_t pin,pout;

    struct mq_attr attr;
	attr.mq_maxmsg=10;
	attr.mq_msgsize=1;


	if((pout=TEMP_FAILURE_RETRY(mq_open("/bingo_out", O_RDWR | O_CREAT, 0600, &attr)))==(mqd_t)-1) //O_NONBLOCK
        ERR("mq open out");
    if((pin=TEMP_FAILURE_RETRY(mq_open("/bingo_in", O_RDWR | O_CREAT | O_NONBLOCK, 0600, &attr)))==(mqd_t)-1)
        ERR("mq open in");

    sethandler(sigchld_handler,SIGCHLD);
	sethandler(mq_handler,SIGRTMIN);

    create_children(n, pout, pin);


    static struct sigevent not;

	not.sigev_notify=SIGEV_SIGNAL;
	not.sigev_signo=SIGRTMIN;
	not.sigev_value.sival_ptr=&pin;

	if(mq_notify(pin, &not)<0)
        ERR("mq_notify");
    parent_work(pout);
    
    mq_close(pout);
    mq_close(pin);

    if(mq_unlink("/bingo_out"))
        ERR("mq unlink");
    if(mq_unlink("/bingo_in"))
        ERR("mq unlink");

	return EXIT_SUCCESS;
}