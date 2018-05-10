#include <pthread.h>
#include <signal.h>
#include <pthread.h>

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#include "tm/test_threads.hpp"
#include "tm/Invyswell.hpp"
#include "tm/rand_r_32.h"

#include <errno.h>

uint64_t* accountsAll;
#define ACCOUT_NUM 1048576

int cnt[300];

/**
 *  Support a few lightweight barriers
 */
void
barrier(uint32_t which)
{
    static volatile uint32_t barriers[16] = {0};
    CFENCE;
    __sync_fetch_and_add(&barriers[which], 1);
    while (barriers[which] != total_threads) { }
    CFENCE;
}

inline unsigned long long get_real_time(void)
{
	struct timespec time;
	clock_gettime(CLOCK_MONOTONIC_RAW, &time);

	return time.tv_sec * 1000000000L + time.tv_nsec;
}

void* th_run(void * args)
{
	int id = ((long)args);
    	
    uint64_t* accounts = accountsAll;
	
	/* tx_id is thread-local variable */
	tx_id = id;

	/* initialize write_set, read_set, and commit_lock */
    thread_init(id);

	int tm_cnt[6];
	for (int i = 0; i < 6; i++)
		tm_cnt[i] = 0;

	uint64_t localCounter = 0;
	int fail_fast_log = -1;

    barrier(0);
	unsigned int seed = id;

	for (int i = 0; i < 100; i++) 
	{
		int acc1[1000];
		int acc2[1000];

		for (int j=0; j< 10; j++)
		{
				acc1[j] = rand_r_32(&seed) % ACCOUT_NUM;
				acc2[j] = rand_r_32(&seed) % ACCOUT_NUM;
		}

		if (tx[tx_id].fail_fast || sw_cnt == 0)
		{
			if (tx[tx_id].fail_fast && fail_fast_log == -1)
				fail_fast_log = i;
			
			tx[tx_id].type = 0;
		}
		else
			tx[tx_id].type = 1;
		
		tx[tx_id].attempts = 5;
		
		for (int j=0; j< 10; j++)
		{
again:
			INVYSWELL_TX_BEGIN
			cnt[id]++;
			if (status == _XBEGIN_STARTED || status == _STM_STARTED)
			{
				invyswell_tx_write(&accounts[acc1[j]], (invyswell_tx_read(&accounts[acc1[j]]) + 50));
				invyswell_tx_write(&accounts[acc2[j]], (invyswell_tx_read(&accounts[acc2[j]]) - 50));
				localCounter++;	
				invyswell_tx_end();
				tm_cnt[tx[tx_id].type]++;
			}
			else if (tx[tx_id].attempts > 0)
			{
				tx[tx_id].attempts--;
				goto again;
			}
			else
			{
				/* fast-fail mode */
				if (tx[tx_id].fail_fast)
					tx[tx_id].type = 5;
				else
					tx[tx_id].type++;
		
				tx[tx_id].attempts = 5;
				goto again;
			}
		}
	}
 	printf("Thread %d local counter = %lu, LightHW = %d, BFHW = %d, SpecSW = %d, IrrevocSW = %d, SglSW = %d, racy_shared: %ld, fail-fast happens on %d\n", \
			id, localCounter, tm_cnt[0], tm_cnt[1], tm_cnt[2], tm_cnt[3], tm_cnt[5], tx[tx_id].racy_shared, fail_fast_log); 

	return 0;
}

int main(int argc, char* argv[])
{

	tm_sys_init();

	if (argc < 2) {
		printf("Usage test threads#\n");
		exit(0);
	}

    int th_per_zone = atoi(argv[1]);
	total_threads = th_per_zone? th_per_zone : 1;

	accountsAll = (uint64_t*) malloc(sizeof(uint64_t) * ACCOUT_NUM);

	long initSum = 0;
	for (int i=0; i<ACCOUT_NUM; i++) {
		accountsAll[i] = 100;
	}
	for (int i=0; i<ACCOUT_NUM; i++) {
		initSum += accountsAll[i];
	}
	printf("init sum = %ld\n", initSum);

	pthread_attr_t thread_attr;
	pthread_attr_init(&thread_attr);

	pthread_t client_th[300];
	int ids = 1;
	for (unsigned long i = 1; i < (unsigned long)th_per_zone; i++) {
		pthread_create(&client_th[ids-1], &thread_attr, th_run, (void*)i);
		ids++;
	}
	
	unsigned long long start = get_real_time();
	th_run(0);

	for (int i=0; i<ids-1; i++) {
		pthread_join(client_th[i], NULL);
	}

	printf("\nTime: %lld ", get_real_time() - start);
	
	long sum = 0;
	int c=0;
	for (int i=0; i<ACCOUT_NUM; i++) {
		sum += accountsAll[i];
		if (accountsAll[i] != 100) {
			c++;
		}
	}

	int total_cnt = 0;
	printf("sum = %ld, matched = %d changed %d\n", sum, sum == initSum, c);
	for (int i=0; i < total_threads; i++)
		total_cnt += cnt[i];

	printf("total_cnt: %d\n", total_cnt);

	return 0;
}
