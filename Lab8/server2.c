//File sending through TCP - server side
//Commented
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <signal.h>
#include <netdb.h>
#include <pthread.h>
//Error macro
#define ERR(source) (perror(source),\
		     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
		     exit(EXIT_FAILURE))

//Define number of awaiting connections, chunk to send,
//filename length and number of threads in the pool
#define BACKLOG 3
#define CHUNKSIZE 500
#define NMMAX 30
#define THREAD_NUM 3

//Define error string to show
#define ERRSTRING "No such file or directory\n"

//Signal handling part
volatile sig_atomic_t work = 1;

void siginthandler(int sig) {
    work = 0;
}

void sethandler(void (*f)(int), int sigNo) {
    struct sigaction act;
    memset(&act, 0x00, sizeof(struct sigaction));
    act.sa_handler = f;
    
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}
//End of signal handling part

//Thread arguments structure
typedef struct {
	int id;
	int *idlethreads;
	int *socket;
	int *condition;
	pthread_cond_t *cond;
	pthread_mutex_t *mutex;
} thread_arg;

//Usage
void usage(char *name) {
	fprintf(stderr, "USAGE: %s port workdir\n",name);
	exit(EXIT_FAILURE);
}

//Library function: bulk read
ssize_t bulk_read(int fd, char *buf, size_t count) {
	int c;
	size_t len = 0;
	do {
		c = TEMP_FAILURE_RETRY(read(fd, buf, count));
		if (c < 0) return c;
		if (c == 0) return len;
		buf += c;
		len += c;
		count -= c;
	}
	while (count > 0);
	return len;
}

//Library function: bulk write
ssize_t bulk_write(int fd, char *buf, size_t count) {
	int c;
	size_t len = 0;
	do {
		c = TEMP_FAILURE_RETRY(write(fd, buf, count));
		if(c < 0) return c;
		buf += c;
		len += c;
		count -= c;
	}
	while (count > 0);
	return len;
}

//Agnostic socket creation
int make_socket(int domain, int type) {
	int sock;
	sock = socket(domain, type, 0);
	if (sock < 0)
        ERR("socket");
	return sock;
}

//Communication function for a given client
void communicate(int clientfd) {
	int fd;
	ssize_t size;
	char filepath[NMMAX+1];
	char buffer[CHUNKSIZE];
    //Get filepath from the client - waits until everything was received from the client
	if ((size = TEMP_FAILURE_RETRY(recv(clientfd, filepath, NMMAX + 1, MSG_WAITALL))) == -1)
		ERR("read");
    //If we read enough data
	if (size == NMMAX + 1) {
        //Open file - if we can't, then put error string into the buffer
		if ((fd = TEMP_FAILURE_RETRY(open(filepath, O_RDONLY))) == -1)
			sprintf(buffer, ERRSTRING);
		else {
            //Otherwise, read the file into the buffer
			memset(buffer, 0x00, CHUNKSIZE);
			if ((size = bulk_read(fd, buffer, CHUNKSIZE)) == -1)
                ERR("read");
		}
        //Try to send the chunk (or error message) back to client
		if (TEMP_FAILURE_RETRY(send(clientfd, buffer, CHUNKSIZE, 0)) == -1)
            ERR("write");
	}
    
    //Close client's file descriptor
	if (TEMP_FAILURE_RETRY(close(clientfd)) < 0)
        ERR("close");
}

//Cleanup function
void cleanup(void *arg) {
    //Unlock pthread mutex from provided argument
	pthread_mutex_unlock((pthread_mutex_t *)arg);
}

//Starting function for all pthreads
void *threadfunc(void *arg) {
	int clientfd;
	thread_arg targ;
	memcpy(&targ, arg, sizeof(targ)); //Create a local copy of passed thread_arg
	while (1)
	{
        //Put cleanup function on cleanup stack (to use later)
		pthread_cleanup_push(cleanup, (void *) targ.mutex);
        //Lock the mutex, block if it was already blocked
		if (pthread_mutex_lock(targ.mutex) != 0)
			ERR("pthread_mutex_lock");
        //Set thread as waiting to accept
		(*targ.idlethreads)++;
        //While SIGINT wasn't called and condition is set to 0 (no job):
        //wait on mutex until condition is signalled (job is ready)
		while (!*targ.condition && work)
			if (pthread_cond_wait(targ.cond, targ.mutex) != 0)
				ERR("pthread_cond_wait");
        //Reset condition
        //Condition = 0 means work taken up, nothing waiting for it
		*targ.condition = 0;
        //Exit if SIGINT was handled
		if (!work)
			pthread_exit(NULL);
        //Set thread as busy
		(*targ.idlethreads)--;
        //Get socket to use
		clientfd = *targ.socket;
        //Unlock a recently waiting thread so that it can wait to accept
		pthread_cleanup_pop(1);
        //Communicate using socket that was received
		communicate(clientfd);
	}
	return NULL;
}

//Pthread pool initialization function
void init(pthread_t *thread, thread_arg *targ, pthread_cond_t *cond, pthread_mutex_t *mutex, int *idlethreads, int *socket, int *condition) {
	int i;
    //Put into provided array of thread_arg
	for (i = 0; i < THREAD_NUM; i++) {
		targ[i].id = i + 1;
        //Things below are defaults for/shared by all threads
		targ[i].cond = cond;
		targ[i].mutex = mutex;
		targ[i].idlethreads = idlethreads;
		targ[i].socket = socket;
		targ[i].condition = condition;
        //Create a thread starting in threadfunc(&targ[i])
		if (pthread_create(&thread[i], NULL, threadfunc, (void *) &targ[i]) != 0)
			ERR("pthread_create");
	}
}

//Bind a TCP socket at a given port
int bind_tcp_socket(uint16_t port) {
	struct sockaddr_in addr;
	int socketfd, t=1;
	socketfd = make_socket(PF_INET, SOCK_STREAM);
	memset(&addr, 0x00, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(t)))
        ERR("setsockopt");
	if (bind(socketfd, (struct sockaddr *) &addr, sizeof(addr)) < 0)
        ERR("bind");
	if (listen(socketfd, BACKLOG) < 0)
        ERR("listen");
	return socketfd;
}

//Add new client
int add_new_client(int sfd) {
	int nfd;
    //Try to get next client
	if ((nfd = TEMP_FAILURE_RETRY(accept(sfd, NULL, NULL))) < 0) {
        if (EAGAIN == errno || EWOULDBLOCK == errno) //If no client available: return -1
            return -1;
		ERR("accept");
	}
	return nfd;
}

void dowork(int socket, pthread_t *thread, thread_arg *targ, pthread_cond_t *cond, pthread_mutex_t *mutex, int *idlethreads, int *cfd, sigset_t *oldmask, int *condition) {
	int clientfd;
	fd_set base_rfds, rfds;
    //Initialize pselect list
	FD_ZERO(&base_rfds);
	FD_SET(socket, &base_rfds);
    //Do until SIGINT happens
	while (work) {
		rfds = base_rfds; //Copy pselect list (so we reuse it later)
        //Check if socket is readable, allow SIGINT handling
		if (pselect(socket + 1, &rfds, NULL, NULL, NULL, oldmask) > 0) {
            //If there's no client waiting: try again
			if ((clientfd = add_new_client(socket)) == -1)
				continue;
            //If mutex can't be locked: crash
			if (pthread_mutex_lock(mutex) != 0)
				ERR("pthread_mutex_lock");
            //If no threads are waiting to be used: close the client socket and unlock mutex
			if (*idlethreads == 0) {
				if (TEMP_FAILURE_RETRY(close(clientfd)) == -1)
                    ERR("close");
				if (pthread_mutex_unlock(mutex) != 0)
                    ERR("pthread_mutex_unlock");
			}
            //Otherwise: set found client as global file descriptor, unlock mutex
            //and unblock one of threads waiting on condition
			else {
				*cfd = clientfd;
				if (pthread_mutex_unlock(mutex) != 0)
                    ERR("pthread_mutex_unlock");
                //condition = 1 means "work to do"
				*condition = 1;
				if (pthread_cond_signal(cond) != 0)
                    ERR("pthread_cond_signal");
			}
        }//If pselect failed: try again if SIGINT caused this, crash if not
		else {
			if (EINTR == errno)
                continue;
            ERR("pselect");
		}
	}
}
int main(int argc, char **argv) {
	int i, condition = 0, socket, new_flags, cfd, idlethreads = 0;
	pthread_t thread[THREAD_NUM];
	thread_arg targ[THREAD_NUM];
    //Initialize condition variable and mutex
	pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	sigset_t mask, oldmask;
	if (argc!=3)
        usage(argv[0]);
    //Change current directory to one provided, crash if not possible
	if (chdir(argv[2]) == -1)
        ERR("chdir");
	sethandler(SIG_IGN, SIGPIPE); //Ignore SIGPIPE
	sethandler(siginthandler, SIGINT); //Handle SIGINT
    //Block SIGINT, save old mask for pselect
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigprocmask(SIG_BLOCK, &mask, &oldmask);
    //Bind TCP socket, set it as non-blocking
	socket = bind_tcp_socket(atoi(argv[1]));
	new_flags = fcntl(socket, F_GETFL) | O_NONBLOCK;
	if (fcntl(socket, F_SETFL, new_flags) == -1)
        ERR("fcntl");
    //Initialize array of thread_arg
	init(thread, targ, &cond, &mutex, &idlethreads, &cfd, &condition);
    //Do server work
	dowork(socket, thread, targ, &cond, &mutex, &idlethreads, &cfd, &oldmask, &condition);
    //Broadcast means UNLOCK ALL waiting on condition - so that they can terminate
	if (pthread_cond_broadcast(&cond) != 0)
        ERR("pthread_cond_broadcast");
    //Wait for all pthreads to terminate
	for (i = 0; i < THREAD_NUM; i++)
		if (pthread_join(thread[i], NULL) != 0)
            ERR("pthread_join");
    //Close socket
	if (TEMP_FAILURE_RETRY(close(socket)) < 0)
        ERR("close");
	return EXIT_SUCCESS;
}
