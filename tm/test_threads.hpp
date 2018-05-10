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

#define CFENCE  __asm__ volatile ("":::"memory")
#define MFENCE  __asm__ volatile ("mfence":::"memory")

#define FORCE_INLINE __attribute__((always_inline)) inline

#define RACY_THRESHOLD 150
#define ACCESS_SIZE 102400
#define FILTER_SIZE 4096
#define WC  3
#define RC  1
#define FCC 3
#define SC  3

#define IS_LOCKED(lock) lock & 1 == 1
#define UNLOCK(lock) lock = 0
#define GET_VERSION(lock) lock >> 1
#define SET_VERSION(lock, new_ver) lock = ((new_ver << 1) | 1)
#define TRY_LOCK(lock) __sync_bool_compare_and_swap(&(lock), (lock) & ~1, lock | 1)

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
volatile unsigned int commit_lock;
unsigned long hw_post_commit;

thread_local int tx_id;

struct Tx_Context
{
	int id;
	jmp_buf scope;
	BitFilter<FILTER_SIZE> write_filter;
	BitFilter<FILTER_SIZE> read_filter;
	WriteSet *write_set;
	WriteSet *read_set;
	int status;
	bool inflight;
	int priority;
	int type;
	int attempts;
	uint64_t local_cs;
	int fail_fast;
	uint64_t racy_shared;
};

struct Tx_Context tx[300];


FORCE_INLINE void thread_init(int id){
	tx[id].id = id;
	tx[id].write_set = new WriteSet(ACCESS_SIZE);
	tx[id].read_set = new WriteSet(ACCESS_SIZE);
	tx[id].status = VALID;
	tx[id].inflight = false;
	tx[id].racy_shared = 0;
	tx[id].fail_fast = 0;
}

FORCE_INLINE void tm_sys_init(){
	commit_lock = 0;
	commit_sequence = 0;
	sw_cnt = 0;
	hw_post_commit = 0;
	total_threads = 0;
}

void fuck_barrier(int which)
{
	static volatile int barriers[16] = {0};
	CFENCE;
	__sync_fetch_and_add(&barriers[which], 1);
	while(barriers[which] != total_threads) {}
	CFENCE;
}
#endif
