#include "rtm.h"
#include "BitFilter.h"
#include <threads.h>

#define FILTER_SIZE 4096
#define WC = 3;
#define RC = 1;
#define FCC = 3;
#define SC = 3;

enum Tx_Stauts
{
	INVALID,
	VALID
};

/*Global Variables*/
unsigned long commit_sequence;
unsigned long sw_cnt;
pthread_mutex_t commit_lock;
unsigned long hw_post_commit;

thread_local int tx_id;

struct Tx_Context
{
	int id;
	jmp_buf scope;
	BitFilter<FILTER_SIZE> write_filter;
	BitFilter<FILTER_SIZE> read_filter;
	WriteSet *write_set;
	ReadSet *read_set;
	unsigned long local_cs;
	int status;
	bool isInFlight;
	unsigned long priority;
}

struct Tx_context tx[300];

/* BFHW functions */
void BFHW_tx_read(void);
void BFHW_tx_write(void);
void BFHW_tx_end(void);
void BFHW_tx_post_commit(void);
