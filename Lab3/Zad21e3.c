#include <stdlib.h>
#include <stddef.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#define MAXLINE 4096
#define DEFAULT_THREADCOUNT 10
#define DEFAULT_SAMPLESIZE 100

#define ERR(source) (perror(source),\
		     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
		     exit(EXIT_FAILURE))

volatile sig_atomic_t last_signal = 0;

void handler(int dummy);
void sethandler( void (*f)(int), int sigNo) {
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (-1==sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}


typedef unsigned int UINT;
typedef struct argsEstimation {
	pthread_t tid;
	UINT seed;
	int divide;
    int *counter;
} args_t;

void ReadArguments(int argc, char **argv, int *threadCount);
void* job(void *args);
int main(int argc, char** argv) {
	int threadCount;
    int count = 1;
	ReadArguments(argc, argv, &threadCount);
    args_t* data = (args_t*) malloc(sizeof(args_t) * threadCount);
    if (data == NULL)
        ERR("Malloc error for estimation arguments!");
    srand(time(NULL));
    for (int i = 0; i < threadCount; i++) {
        data[i].counter = &count;
		data[i].seed = rand();
	}

    sethandler(handler, SIGINT);

    for(int i = 0; i<threadCount; i++){
		data[i].divide = (rand_r(&data[i].seed) % (100 + 1 - 2)) + 2;
	}
    for (int i = 0; i < threadCount; i++) {
		int err = pthread_create(&(data[i].tid), NULL, job, &data[i]);
        if (err != 0)
            ERR("Couldn't create thread");
	}
 
    while(last_signal==0){
		nanosleep((const struct timespec[]){{0, 100000000L}}, NULL);
		count++;
	}

    for (int i = 0; i < threadCount; i++) {
		int err = pthread_join(data[i].tid, NULL);
        if (err != 0)
            ERR("Can't join with a thread");
    }
    printf("\n");    
}

void ReadArguments(int argc, char **argv, int *threadCount) {
	*threadCount = DEFAULT_THREADCOUNT;

	if (argc >= 2) {
		*threadCount = atoi(argv[1]);
		if (*threadCount <= 0) {
			printf("Invalid value for 'threadCount'");
			exit(EXIT_FAILURE);
		}
	}
}
void* job(void *voidPtr){
    args_t *args = voidPtr;
    while(1){
		if(last_signal)
			break;
        if((*args->counter % args->divide)==0)
            printf("%d jest podzielne przez %d\n", *args->counter, args->divide);
    }
}

void handler(int dummy){
	last_signal = 1;
    printf("\nYou hit Ctrl+C\n");
}