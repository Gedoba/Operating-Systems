#include <stdlib.h>
#include <stddef.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#define MAXLINE 4096
#define DEFAULT_THREADCOUNT 10
#define DEFAULT_ARRAY_SIZE 2

#define ERR(source) (perror(source),\
		     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
		     exit(EXIT_FAILURE))

typedef unsigned int UINT;

typedef struct argsEstimation {
	UINT seed;
	pthread_t tid;
	int array_size_arg;
	float *array_arg;
	pthread_mutex_t *pmx_arg;
	pthread_mutex_t *pmxCheck_arg;
	int* count;
	int* check;
} args_t;

void ReadArguments(int argc, char** argv, int* threads, int* array_size){
	*threads = DEFAULT_THREADCOUNT;
	*array_size = DEFAULT_ARRAY_SIZE;
	if(argc>=2){
		*threads = atoi(argv[1]);
		if(*threads < 1){
			printf("invalid number of threads\n");
			exit(EXIT_FAILURE);
		}
	}
	if(argc>=3){
		*array_size = atoi(argv[2]);
		if(*array_size < 1){
			printf("invalid size of the array\n");
			exit(EXIT_FAILURE);
		}
	}
}

void* thread_work(void *v){
	args_t *args = v;
	while(*args->count < args->array_size_arg){
		nanosleep((const struct timespec[]){{0, 100000000L}}, NULL);
		int r = rand_r(&args->seed)%args->array_size_arg;
		pthread_mutex_lock(&args->pmx_arg[r]);
		if(args->check[r]){	
			float f = sqrt(args->array_arg[r]);
			args->array_arg[r] = f;
			args->check[r] = 0;
			printf("cell %d changed\n", r);
			*args->count=*args->count+1;
		}
		pthread_mutex_unlock(&args->pmx_arg[r]);
	}
	return NULL;
}

void print_arr(float* array, int array_size){
	printf("\nYour array:\n");
	for(int i = 0; i<array_size; i++){
		printf("%f ", array[i]);
	}
	printf("\n\n");
}

int main (int argc, char** argv){
	int threads, array_size = 0, counter = 0;

	ReadArguments(argc, argv, &threads, &array_size);

	pthread_mutex_t* pmx = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t)*array_size);
	for(int i = 0; i<array_size; i++){
		if(pthread_mutex_init(&pmx[i], NULL)) ERR("Couldn't initialize mutex!");
	}

	int* checking = (int*) malloc(sizeof(int)*array_size);
	for(int i = 0; i<array_size; i++){
		checking[i] = 1;
	}

	float* array = (float*) malloc(sizeof(float)*array_size);
	float a = 59.00;
	srand(time(NULL));
	for(int i = 0; i< array_size;i++){
		array[i] = (((float)rand()/(float)(RAND_MAX)) * a)+1;
	}

	print_arr(array, array_size);

	args_t* data = (args_t*) malloc(sizeof(args_t)*threads);
	if(data == NULL) ERR("malloc error");

	for(int i = 0; i<threads; i++){
		data[i].array_size_arg = array_size;
		data[i].array_arg = array;
		data[i].seed = rand();
		data[i].pmx_arg = pmx;
		data[i].count = &counter;
		data[i].check = checking;
	}

	for(int i = 0; i<threads; i++){
		int err = pthread_create(&(data[i].tid), NULL, thread_work, &data[i]);
		if(err!=0) ERR("thread creation error");
	}

	for(int i = 0; i<threads; i++){
		int err = pthread_join(data[i].tid, NULL);
		if(err!=0) ERR("thread joining error");
	}

	print_arr(array, array_size);

	exit(EXIT_SUCCESS);
}