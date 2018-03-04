#include <stdlib.h>
#include <stddef.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <stdarg.h>
#include <signal.h>

#define DEFAULT_N 10
#define MAX_N 100
#define ERR(source) (perror(source),\
		     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
		     exit(EXIT_FAILURE));

typedef unsigned int UINT;
typedef struct timespec timespec_t;
typedef struct watek_dane
{
	int n;
	pthread_t t;
	int pos1;
	int pos2;
	int *tab;
	UINT seed;	
    //pthread_mutex_t *mx1;
    //pthread_mutex_t *mx2;
}watek_dane;


void * thread_work(void *arg);
void msleep(UINT milisec);

int main (int argc, char ** argv)
{
	if(argc!=2)
	{
		printf("potrzeba jednego parametru - k\n");
		exit(EXIT_FAILURE);
	}
	int k = atoi(argv[1]);
	watek_dane* dane;
	dane = (watek_dane *)malloc(sizeof(watek_dane)*k);
	if(!dane)
	{
		ERR("malloc");
	}
	int i = 0;
	int *tab = (int *)malloc(sizeof(int)*MAX_N);
	if(!tab)
	{
		ERR("malloc");
	}
	while(i<MAX_N&&fscanf(stdin,"%d",&tab[i])!= EOF)++i;
	int n = i;
	printf("wczytalem %d liczb\n",n);
	srand(time(NULL));
    for (i = 0; i < k; ++i)
    {
		dane[i].n = n;
		dane[i].tab = tab;
		dane[i].seed = rand();
    }
    for (i = 0; i < k; ++i) 
    {
         int err = pthread_create(&(dane[i].t), NULL, thread_work, &dane[i]);
         if (err != 0) ERR("Couldn't create thread");
    }
    
    for (int i = 0; i < k; i++) 
    {
          int err = pthread_join(dane[i].t, NULL);
          if (err != 0) ERR("Can't join with a thread");
	}
	printf("wszystkie wątki zakończone\n");
	for(i = 0; i < n; ++i)
	{
		printf("%d\n",tab[i]);
	}
	free(dane);
	printf("KONIEC watku glownego\n");
	exit(EXIT_SUCCESS);
}

void * thread_work(void *arg)
{
	watek_dane* dane = arg;
	int pos1 = rand_r(&(dane->seed))%dane->n;
	int pos2 = rand_r(&(dane->seed))%dane->n;
	if(pos1 > pos2)
	{
		int tmp = pos1;
		pos1 = pos2;
		pos2 = tmp;
	}
	printf("pozycja1=%d liczba=%d\n",pos1,dane->tab[pos1]);	
	printf("pozycja2=%d liczba=%d\n",pos2,dane->tab[pos2]);		
	UINT spanie = rand_r(&(dane->seed)) % 1501 + 1500;
	printf("bede spac %d milisekund\n", spanie);
    msleep(spanie);
    //if(pthread_mutex_init(dane->mx1, NULL))ERR("Couldn't initialize mutex!");
    //if(pthread_mutex_init(dane->mx2, NULL))ERR("Couldn't initialize mutex!");
    //pthread_mutex_t mx1 = PTHREAD_MUTEX_INITIALIZER;;
    //pthread_mutex_t mx2 = PTHREAD_MUTEX_INITIALIZER;;
    //printf("%d %d %d\n", dane->tab[pos1] , dane->tab[pos2],(dane->tab[pos1]) < (dane->tab[pos2]));
    if((dane->tab[pos1]) < (dane->tab[pos2]))
    {
		int tmp = dane->tab[pos1];
		dane->tab[pos1]=dane->tab[pos2];
		dane->tab[pos2]=tmp;
	}
	//printf("pozycja1=%d liczba=%d\n",pos1,dane->tab[pos1]);	
	//printf("pozycja2=%d liczba=%d\n",pos2,dane->tab[pos2]);		
	return NULL;
}

void msleep(UINT milisec) 
{
    time_t sec= (int)(milisec/1000);
    milisec = milisec - (sec*1000);
    timespec_t req= {0};
    req.tv_sec = sec;
    req.tv_nsec = milisec * 1000000L;
    if(nanosleep(&req,&req)) ERR("nanosleep");
}