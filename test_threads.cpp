#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#include <errno.h>
#include "tm/test_threads.hpp"
#include "tm/Invyswell.hpp"

#define CFENCE  __asm__ volatile ("":::"memory")
#define MFENCE  __asm__ volatile ("mfence":::"memory")

//int total_threads = 0;
//unsigned long commit_sequence = 0;
//unsigned long sw_cnt = 0;
//pthread_mutex_t commit_lock = PTHREAD_MUTEX_INITIALIZER;
//unsigned long hw_post_commit = 0;
//bool canAbort = false;

uint64_t counter = 0;

inline unsigned long long get_real_time() {
        struct timespec time;
    clock_gettime(CLOCK_MONOTONIC_RAW, &time);

    return time.tv_sec * 1000000000L + time.tv_nsec;
}

/**
 *  Support a few lightweight barriers
 */
void
barrier(int which)
{
    static volatile int barriers[16] = {0};
    CFENCE;
    __sync_fetch_and_add(&barriers[which], 1);
    while (barriers[which] != total_threads) { }
    CFENCE;
}

void
signal_callback_handler(int signum)
{
   // Terminate program
   exit(signum);
}

volatile bool ExperimentInProgress = true;
static void catch_SIGALRM(int sig_num)
{
    ExperimentInProgress = false;
}

void* th_run(void * args)
{

	long id = (long)args;
	/* tx_id is thread-local variable */
	tx_id = id;

	/* initialize write_set, read_set, and commit_lock */
	thread_init((int)id);

	uint64_t localCounter = 0;

	barrier(0);
		
	int inHTM = 0;
	int inSTM = 0;

	for (int i=0; i<1000; i++) 
	{
		tx[tx_id].type = 2;
		tx[tx_id].attempts = 5;
again:	
		INVYSWELL_TX_BEGIN
		if (status == _XBEGIN_STARTED || status == _STM_STARTED)
		{
			uint64_t value = invyswell_tx_read(&counter);
			invyswell_tx_write(&counter, value + 1);
			localCounter++;	
			invyswell_tx_end();
			inHTM++;
		}
		else if (tx[tx_id].attempts > 0)
		{
			//printf("1: inHTM:%d\n", inHTM);
			tx[tx_id].attempts--;
			goto again;
		}
		else
		{
			//printf("2: inHTM:%d\n",inHTM);
#if 0
			if (tx[tx_id].type == 1)
			{
				tx[tx_id].attempts = 5;
				goto again;
			}
#endif
			//tx[tx_id].type++;
			tx[tx_id].attempts = 5;
			goto again;
		}
	}
	printf("commit_sequence: %lu\n", commit_sequence);
 	printf("Thread %ld local counter = %lu and global counter = %lu, inHTM = %d, inSTM = %d\n", id, localCounter, counter, inHTM, inSTM); 
	return 0;
}

int main(int argc, char* argv[])
{
//	signal(SIGINT, signal_callback_handler);

	if (argc < 2) {
		printf("Usage test threads#\n");
		exit(0);
	}
	
	tm_sys_init();

    total_threads = atoi(argv[1]);

	pthread_attr_t thread_attr;
	pthread_attr_init(&thread_attr);

	pthread_t client_th[300];
	long ids = 1;
	for (int i = 1; i<total_threads; i++) {
		pthread_create(&client_th[ids-1], &thread_attr, th_run, (void*)ids);
		ids++;
	}
	
	unsigned long long start = get_real_time();

	th_run(0);

	for (int i=0; i<ids-1; i++) {
		pthread_join(client_th[i], NULL);
	}
	
	printf("Total time = %lld ns\n", get_real_time() - start);
	printf("Counter = %lu\n", counter);

	return 0;
}

//Build with 
//g++ test_threads.cpp -o test -lpthread
