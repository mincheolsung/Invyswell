#ifndef __INVYSWELL_H__
#define __INVYSWELL_H__

#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include <setjmp.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "rtm.h"
#include "BitFilter.h"
#include "WriteSet.hpp"

#define FORCE_INLINE __attribute__((always_inline)) inline

#define ACCESS_SIZE 102400
#define FILTER_SIZE 4096
#define WC  3
#define RC  1
#define FCC 3
#define SC  3

using stm::WriteSetEntry;
using stm::WriteSet;

enum Tx_Stauts
{
	INVALID,
	VALID
};

/*Global Variables*/
int total_threads;
unsigned long commit_sequence;
unsigned long sw_cnt;
pthread_mutex_t commit_lock;
unsigned long hw_post_commit;
//bool canAbort;

thread_local int tx_id;

struct Tx_Context
{
	int id;
	jmp_buf scope;
	BitFilter<FILTER_SIZE> write_filter;
	BitFilter<FILTER_SIZE> read_filter;
	WriteSet *write_set;
	WriteSet *read_set;
	unsigned long local_cs;
	int status;
	bool inflight;
	int priority;
	int type;
	int attempts;
};

struct Tx_Context tx[300];


FORCE_INLINE void thread_init(int id){
	tx[id].id = id;
	tx[id].write_set = new WriteSet(ACCESS_SIZE);
	tx[id].read_set = new WriteSet(ACCESS_SIZE);
	tx[id].status = VALID;
}

FORCE_INLINE void tm_sys_init(){
	commit_sequence = 0;
	sw_cnt = 0;
	hw_post_commit = 0;
}
/*
stm::WriteSet::WriteSet(const size_t initial_capacity)
{
//	tx[tx_id].write_set = new WriteSet(ACCESS_SIZE);
//	tx[tx_id].read_set = new WriteSet(ACCESS_SIZE);
	if (pthread_mutex_init(&commit_lock, NULL) != 0)
		printf("commit_lock init fails\n");
}
*/
#endif
