#ifndef __INVYSWELL_H__
#define __INVYSWELL_H__

#include "rtm.h"
#include "BitFilter.h"
#include "WriteSet.hpp"
#include <pthread.h>

#define FORCE_INLINE __attribute__((always_inline)) inline

#define FILTER_SIZE 4096
#define WC = 3;
#define RC = 1;
#define FCC = 3;
#define SC = 3;

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
bool canAbort;

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
	bool isInFlight;
	int priority;
};

struct Tx_Context tx[300];

/* BFHW functions */
void BFHW_tx_read(void);
void BFHW_tx_write(void);
void BFHW_tx_end(void);
void BFHW_tx_post_commit(void);

#endif
