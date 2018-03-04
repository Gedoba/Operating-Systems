#include <stdlib.h>
#include <stddef.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#define MAXLINE 4096
#define DEFAULT_THREADCOUNT 3
#define DEFAULT_ARRAYSIZE 7

#define ERR(source) (perror(source),\
		     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
		     exit(EXIT_FAILURE))

typedef unsigned int UINT;
typedef struct argsSqrt {
	pthread_t tid;
	UINT seed;
	int arraySize;
    double* array;
    int *counter;
    int *check;
    pthread_mutex_t *pmx_arg;
} argsSqrt_t;

void ReadArguments(int argc, char **argv, int *threadCount, int *arraySize);
void printArray(double* arr, int arr_size);
void* job(void* voidPtr);

int main(int argc, char** argv) {
    int threadCount, arraySize;
    int count = 0;
    int *checked;
	ReadArguments(argc, argv, &threadCount, &arraySize);

    argsSqrt_t* squareRoots = (argsSqrt_t*) malloc(sizeof(argsSqrt_t) * threadCount);
	 if (squareRoots == NULL)
        ERR("Malloc error for squareRoots arguments!");
    pthread_mutex_t* pmx = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t)*arraySize);
	for(int i = 0; i<arraySize; i++){
		if(pthread_mutex_init(&pmx[i], NULL))
            ERR("Couldn't initialize mutex!");
	}

    double* taskArray;
    taskArray = (double*) malloc(arraySize*sizeof(double));
    if(taskArray==NULL)
        ERR("Malloc err for taskArray");
    checked = (int*) malloc(arraySize*sizeof(int));
    if(taskArray==NULL)
        ERR("Malloc err for checked");
    for(int i = 0; i<arraySize; i++){
        checked[i] = 0; //not checked
    }
    
    srand(time(NULL));
	for (int i = 0; i < threadCount; i++) {
		squareRoots[i].seed = rand();
        squareRoots[i].counter = &count;
        squareRoots[i].check = checked; //firstly all of threads are unchecked
		squareRoots[i].arraySize = arraySize;
        squareRoots[i].array = taskArray;
        squareRoots[i].pmx_arg = pmx;
	}
    for(int i=0; i<arraySize; i++){
        taskArray[i] = (((double)rand()/(double)(RAND_MAX)) * 59)+1;
    }
    printArray(taskArray, arraySize);

    for (int i = 0; i < threadCount; i++) {
		int err = pthread_create(&(squareRoots[i].tid), NULL, job, &squareRoots[i]);
        if (err != 0)
            ERR("Couldn't create thread");
	}
	for (int i = 0; i < threadCount; i++) {
		int err = pthread_join(squareRoots[i].tid, NULL);
        if (err != 0)
            ERR("Can't join with a thread");
	}
    printArray(taskArray, arraySize);
    free(taskArray);
    free(squareRoots);
    printf("Counter: %d", count);
    printf("\n\n");

    exit(EXIT_SUCCESS);
}

void ReadArguments(int argc, char **argv, int *threadCount, int *arraySize) {
	*threadCount = DEFAULT_THREADCOUNT;
	*arraySize = DEFAULT_ARRAYSIZE;

	if (argc >= 2) {
		*threadCount = atoi(argv[1]);
		if (*threadCount <= 0) {
			printf("Invalid value for 'threadCount'");
			exit(EXIT_FAILURE);
		}
	}
	if (argc >= 3) {
		*arraySize = atoi(argv[2]);
		if (*arraySize <= 0) {
			printf("Invalid value for 'arraySize'");
			exit(EXIT_FAILURE);
		}
	}
}

void printArray(double* arr, int arr_size){
    printf("\nYour array:\n");
        for(int i=0; i<arr_size; i++){
        printf("%d->%lf, ", i, arr[i]);
    }
    printf("\n");
}

void* job(void* voidPtr){
    argsSqrt_t *args = voidPtr;
    int randInd;
    
    while(*args->counter<args->arraySize){
        nanosleep((const struct timespec[]){{0, 100000000L}}, NULL);
        randInd = rand_r(&args->seed)%args->arraySize;
        pthread_mutex_lock(&args->pmx_arg[randInd]);
        if(args->check[randInd]==0){
            args->array[randInd] = sqrt(args->array[randInd]);
            args->check[randInd] = 1;
            printf("Cell %d changed\n", randInd);
            (*args->counter)++;
        }
        pthread_mutex_unlock(&args->pmx_arg[randInd]);
        
    }
    
    return NULL;
}