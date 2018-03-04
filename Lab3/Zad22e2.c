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
typedef struct argsEstimation {
	pthread_t tid;
	UINT seed;
	int arraySize;
} argsSqrt_t;

void ReadArguments(int argc, char **argv, int *threadCount, int *arraySize);
void* job(void* args);

int main(int argc, char** argv) {
    int threadCount, arraySize;
	ReadArguments(argc, argv, &threadCount, &arraySize);
    argsSqrt_t* squareRoots = (argsSqrt_t*) malloc(sizeof(argsSqrt_t) * threadCount);
	 if (squareRoots == NULL)
        ERR("Malloc error for squareRoots arguments!");


    int* taskArray;
    taskArray = (int*) malloc(arraySize*sizeof(int));
    if(taskArray==NULL)
        ERR("Malloc err for taskArray");
    srand(time(NULL));
	for (int i = 0; i < threadCount; i++) {
		squareRoots[i].seed = rand();
		squareRoots[i].arraySize = arraySize;
	}
    for(int i=0; i<arraySize; i++){
        taskArray[i] = rand() % 60;
        printf("%d->%d, ", i, taskArray[i]);
    
    }

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
    free(taskArray);
    free(squareRoots);
    printf("\n\n");
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

void* job(void* voidPtr){
    argsSqrt_t *args = voidPtr;
    int randInd;
    randInd = rand_r(&args->seed)%args->arraySize;
    printf("randInd: %d, ", randInd);

    return NULL;
}