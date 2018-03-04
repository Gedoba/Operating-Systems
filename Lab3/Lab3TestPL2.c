#include <stdlib.h>
#include <stddef.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <signal.h>

#define MAXLINE 4096
#define DEFAULT_THREADCOUNT 10
#define MAX_N 100
#define ELAPSED(start,end) ((end).tv_sec-(start).tv_sec)+(((end).tv_nsec - (start).tv_nsec) * 1.0e-9)
#define ERR(source) (perror(source),\
		     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
		     exit(EXIT_FAILURE))

typedef unsigned int UINT;
typedef struct timespec timespec_t;
void msleep(UINT milisec);
typedef struct argsSort {
	pthread_t tid;
	UINT seed;
	int pos1;
    int pos2;
    int* arr;
    int arraySize;
    sigset_t *pMask;
    pthread_mutex_t *mx1;
    pthread_mutex_t *mx2;
} argsSort_t;

void ReadArguments(int argc, char **argv, int *threadCount);
void* job(void* args);
void swap(int *xp, int *yp);
void printArray(int* arr, int arr_size);
void* signal_handling(void* voidArgs);

int main(int argc, char** argv) {
    timespec_t start, current;
    if (clock_gettime(CLOCK_REALTIME, &start))
            ERR("Failed to retrieve time!");
    int thread_count;
    ReadArguments(argc, argv, &thread_count);
    argsSort_t* data = (argsSort_t*)malloc(sizeof(argsSort_t)*thread_count);
    if (data == NULL)
        ERR("Malloc error for data arguments!");
    
    sigset_t oldMask, newMask;
        sigemptyset(&newMask);
        sigaddset(&newMask, SIGINT);
         if (pthread_sigmask(SIG_BLOCK, &newMask, &oldMask))
            ERR("SIG_BLOCK error");
	argsSort_t thrsig;
    
    if(pthread_create(&thrsig.tid, NULL, signal_handling, &thrsig))
            ERR("Couldn't create signal handling thread!");
    int* inputArray = (int*)malloc(sizeof(int)*MAX_N);
    if(inputArray == NULL)
        ERR("Malloc for inputArray");

    int i = 0, arraySize;    
    for(i = 0;i<MAX_N; i++){
        if(fscanf(stdin, "%d", &inputArray[i])==EOF)
            break;
    }
    arraySize = i; //inputArray.size();

    pthread_mutex_t* pmx = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t)*arraySize);
	for(int i = 0; i<arraySize; i++){
		if(pthread_mutex_init(&pmx[i], NULL))
            ERR("Couldn't initialize mutex!");
	}
    pthread_mutex_t* pmx2 = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
	
	if(pthread_mutex_init(&pmx2[0], NULL))
        ERR("Couldn't initialize mutex!");

    thrsig.pMask = &newMask;
    thrsig.mx2 = pmx2;
    thrsig.arr = inputArray;
    thrsig.arraySize = arraySize;
    printArray(inputArray, arraySize);
    srand(time(NULL));
    for (i = 0; i < thread_count; i++) {
		data[i].seed = rand();
        data[i].arr = inputArray;
        data[i].arraySize = arraySize;
        data[i].mx1 = pmx;
	}
    

    for (int i = 0; i < thread_count; i++) {
		int err = pthread_create(&(data[i].tid), NULL, job, &data[i]);
        if (err != 0)
            ERR("Couldn't create thread");
	}
	for (int i = 0; i < thread_count; i++) {
		int err = pthread_join(data[i].tid, NULL);
        if (err != 0)
            ERR("Can't join with a thread");
	}


    printArray(inputArray, arraySize);
    free(inputArray);
    free(data);


    if (clock_gettime(CLOCK_REALTIME, &current))
            ERR("Failed to retrieve time!");

    printf("exec. time: %lfs\n", ELAPSED(start, current));
    exit(EXIT_SUCCESS);
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

void* job(void* voidPtr){
    argsSort_t* args = voidPtr;
    int pos1, pos2;

    pos1 = rand_r(&args->seed) % args->arraySize;
    pos2 = rand_r(&args->seed) % args->arraySize;
    while(1)
    {
        if(pos1==pos2){
            pos1 = rand_r(&args->seed) % args->arraySize;
            pos2 = rand_r(&args->seed) % args->arraySize;
        }
        else
            break;
    }
    if(pos1 > pos2)
	{
		int tmp = pos1;
		pos1 = pos2;
		pos2 = tmp;
	}
    UINT sleepTime = rand_r(&args->seed) % (3000-1500+1)+1500;
    printf("SleepTime: %dms\n", sleepTime);
    msleep(sleepTime);
    pthread_mutex_lock(&args->mx1[pos1]);
    if((args->arr[pos1]) < (args->arr[pos2])){
        
        swap(&args->arr[pos1], &args->arr[pos2]);
        
        printf("swapping\n");
    }
    pthread_mutex_unlock(&args->mx1[pos1]);
    printf("Finishing...\n");
    return NULL;
}

void msleep(UINT milisec) 
{
    time_t sec= (int)(milisec/1000);
    milisec = milisec - (sec*1000);
    timespec_t req= {0};
    req.tv_sec = sec;
    req.tv_nsec = milisec * 1000000L;
    if(nanosleep(&req,&req))
        ERR("nanosleep");
}

void swap(int *xp, int *yp)
{
    int temp = *xp;
    *xp = *yp;
    *yp = temp;
}

void printArray(int* arr, int arr_size){
    printf("\nYour array:\n[ ");
        for(int i=0; i<arr_size; i++){
        printf("%d, ", arr[i]);
    }
    printf(" ]\n");
}

void* signal_handling(void* voidArgs) {
        argsSort_t* args = voidArgs;
        int signo;
        srand(time(NULL));
        for (;;) {
                if(sigwait(args->pMask, &signo))
                    ERR("sigwait failed.");
                switch (signo) {
                        case SIGINT:
                                pthread_mutex_lock(args->mx2);
                                printArray(args->arr, args->arraySize);
                                pthread_mutex_unlock(args->mx2);
                                break;
                        default:
                                printf("unexpected signal %d\n", signo);
                                exit(1);
                }
        }
        return NULL;
}