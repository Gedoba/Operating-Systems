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

void sethandler( void (*f)(int, siginfo_t*, void*), int sigNo) {
    
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_sigaction = f;
	act.sa_flags=SA_SIGINFO;
	if (-1==sigaction(sigNo, &act, NULL))
		ERR("sigaction");
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

	for(;;){
		if(mq_receive(*pin,(char*)&ni,1,&msg_prio)<1) {
			if(errno==EAGAIN)
                break;
			else
                ERR("mq_receive");
		}
		if(0==msg_prio)
            printf("MQ: got timeout from %d.\n",ni);
		else
            printf("MQ:%d is a bingo number!\n",ni);
	}

}


void usage(void){
	fprintf(stderr,"USAGE: signals n k p l\n");
	fprintf(stderr,"100 > n > 0 - number of children\n");
	exit(EXIT_FAILURE);
}
void makeQueues(int n, char *queuesNames, mqd_t *queues){
    queuesNames[0] = '/';

    for(int i=1; i<3; i++)
    {
        queuesNames[i]='a';
    }
    queuesNames[3]='\0';

    struct mq_attr attr;
	attr.mq_maxmsg=10;
	attr.mq_msgsize=1;

    for(int i=0; i<n;i++){
        mq_unlink(queuesNames);
        if((queues[i]=TEMP_FAILURE_RETRY(mq_open(queuesNames, O_RDWR|O_NONBLOCK | O_CREAT, 0600, &attr)))==(mqd_t)-1)
            ERR("mq_open");
        else
            printf("Queue name: %s, dzielnik: %d, mkd_t: %d\n",queuesNames, queuesNames[1]-'a'+2, queues[i]); // dzielnik: queues[i]-1
        queuesNames[1] = 'a' + i + 1;
        
    }
}
void sendMessages(int n, char *queuesNames, mqd_t *queues)
{
    srand(getpid());

    for(int i=0; i<5; i++){
        uint8_t raNo = (uint8_t)rand() % 100;
        int queueNo = rand() % n;
        uint8_t rec;
        if(TEMP_FAILURE_RETRY(mq_send(queues[queueNo],(const char*)&raNo,1,0)))
            ERR("mq_send");
        printf("Queue %d No sent: %d\n",queues[queueNo]-1, raNo);
            
        // if(TEMP_FAILURE_RETRY(mq_receive(queues[queueNo],(char*)&rec,1,NULL))<1)
        //     ERR("mq_receive");
        // printf("Queue %d No received: %d\n",queues[queueNo]-1, rec);
    }
}

int main(int argc, char** argv) {
	int n; 
	if(argc!=2)
        usage();
	n = atoi(argv[1]);
	if (n<=0||n>=100)
        usage(); 
    mqd_t *queues = (mqd_t*)malloc(n*sizeof(mqd_t));
    char *queuesNames = (char*)malloc(4*sizeof(char));
    //sleep(2);
    makeQueues(n, queuesNames, queues);
    sendMessages(n, queuesNames, queues);

    for(int i=0; i<n; i++)
    {
        mq_close(queues[i]);
        queuesNames[1] = 'a' + i;
        mq_unlink(queuesNames);
        printf("Queue %s deleted\n", queuesNames);
    }
    free(queues);
    free(queuesNames);
	return EXIT_SUCCESS;
}