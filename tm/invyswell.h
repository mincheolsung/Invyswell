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
	bool inflight;
	int priority;
	int type;
};

struct Tx_Context tx[300];

FORCE_INLINE void invyswell_tx_begin(void)
{
	switch(tx[tx_id].type) {
	/* LightHW */
	case 1:
		_xbegin();
		break;

	/* BFHW */
	case 2:
		_xbegin();
		break;

	/* SpecSW */
	case 3:
		SpecSW_TX_BEGIN
		break;

	/* SglSW */
	case 4:
		SglSW_tx_begin();
		break;

	/* IrrevocSW */
	case 5:
		IrrevocSW_tx_begin();
		break;

	default:
		break;
	}
}

FORCE_INLINE uint64_t invyswell_tx_read(uint64_t *addr)
{
	uint64_t value;

	switch(tx[tx_id].type) {

	/* LightHW */
	case 1:
		value = *addr;	
		break;

	/* BFHW */
	case 2:
		BFHW_tx_read(addr);
		value = *addr;	
		break;

	/* SpecSW */
	case 3:
		value = SpecSW_tx_read(addr);
		break;

	/* SglSW */
	case 4:
		value = *addr;
		break;

	/* IrrevocSW */
	case 5:
		IrrevocSW_tx_read(addr);
		value = *addr;
		break;

	default:
		break;
	}

	return value;
}

FORCE_INLINE void invyswell_tx_write(uint64_t *addr, uint64_t value)
{
	switch(tx[tx_id].type) {

	/* LightHW */
	case 1:
		*addr = value;	
		break;

	/* BFHW */
	case 2:
		BFHW_tx_write(addr);
		*addr = value;	
		break;

	/* SpecSW */
	case 3:
		value = SpecSW_tx_write(addr);
		break;

	/* SglSW */
	case 4:
		*addr = value;
		break;

	/* IrrevocSW */
	case 5:
		IrrevocSW_tx_write(addr);
		*addr = value;
		break;

	default:
		break;
	}
}

FORCE_INLINE void invyswell_tx_end(void)
{

}
