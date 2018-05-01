#include "invyswell.h"

#define  SpecSW_TX_BEGIN(void)					\
{								\
	unsigned long sw_cnt, tx[tx_id].priority = 0;		\
	uint32_t abort_flags = _setjmp (tx.scope);		\
	tx[tx_id].priority++;					\
	__sync_fetch_and_add(&sw_cnt, 1);			\
	do							\
	{							\
		tx[tx_id].local_cs = commit_sequence;		\
	}							\
	while(local_cs & 1);					\
}						

unsigned long SpecSW_tx_read(uint64_t* addr)
{
	WriteSetEntry log((void**)addr);
    	bool found = tx[tx_id].writeset->find(log);
    	if (__builtin_expect(found, true))
		return log.val;	

	tx[tx_id].read_filter.add(addr);
	uint64_t val = *addr;
	validate();
	return val;
} 

void SpecSW_tx_write(uint64_t* addr)
{
	if(tx[tx_id].status == INVALID)
		restart();

	tx[tx_id].write_filter.add(addr);
	//add addr, val to local hash table
	// val = *addr
	tx->write_set->insert(WriteSetEntry((void**)addr, *((uint64_t*)addr)));
	insert(log);

}

void validate()
{
	if(tx[tx_id].local_cs != commit_sequence)
		longjmp(tx[tx_id].scope, 1); // restart

	if(pthread_mutex_trylock(&commit_lock))
		longjmp(tx[tx_id].scope, 1); // restart
	else 
		pthread_mutex_unlock(&commit_lock);

	while(hw_post_commit != 0);

	if(tx[tx_id].status = INVALID)
		longjmp(tx[tx_id].scope, 1); // restart
}

bool iBalance(struct Tx_context *commitTx, struct Tx_context **conflicts, int conflicts_size)
{
	Tx_context *c = commitTx, *tx = NULL;
	int setSize = (c->write-set->size() * (c->priority + WC)) + (c->read_set->size() * (c->priority + RC));

	int abortPrio = 0, abortSetSize = 0, highestPrio = 0;
	bool mostReads = true, mostWrites = true;
	bool fewestCommits = true, can Abort = false;

	for(int i=0; i<conflicts_size; i++)
	{
		tx = conflicts[i];
		if(tx == NULL)
			break;

/*		if(tx->commits < c->commits)
			fewestCommits = false;*/
		if(tx->read_set->size() > tx->read_set->size(c)
			mostReads = false;

		if(tx->write_set->size() > tx->write_set->size(c)
                        mostWritess = false;

		if(tx->priority > highestPrio)
			highestPrio = tx->priority;

		abortSetSize += tx->write_set->size() + tx->read_set->size()
		abortPrio += tx->priority;
	}		

	int p = c->priority;
/*
	if(fewestCommits)
	{
		p = p * FCC;
		setSize = setSize * SC;
	}
*/

	if(p >= highestPrio)
		canAbort = true;

	if(mostReads || mostWrites/* || fewestCommits*/)
		canAbort = true;

	if(setSize >= abortPrio + abortSetSize)
		canAbort = true;

	return canAbort;
}

bool CM_can_commit()
{
	struct Tx_context conflicts[300];
	int conflicts_size;

	//compare write set with read set of in-flight transactions and make set named conflicts
	for(int i=0, conflicts_size=0; i<no_of_threads; i++)
	{ 
		if(tx[i].inflight)
			if(tx[tx_id].write_filter.intersect(tx[i].read_filter))
			{
				conflicts[conflicts_size] =  tx[i];
				conflicts_size++;
			}
	}

	return iBalance(&(tx[tx_id]), conflicts, conflicts_size);
}

void commit(void)
{
	tx[tx_id].write_set->writeback();
}

void SpecSW_tx_end(void)
{
	if(tx[tx_id].writeset->size() == 0) //read-only
	{
		 __sync_fetch_and_sub(&sw_cnt, 1);	
		return;
	}

	pthread_mutex_lock(&commit_lock);
	validate();
	if(!CM_can_commit())
		longjmp(tx[tx_id].scope, 1); // restart

	__sync_fetch_and_sub(&sw_cnt, 1);
	commit();
}

void invalidate()
{
	//compare write set with read set of in-flight transactions and invalidate if match
	for(int i=0; i<no_of_threads; i++)
	{ 
		if(tx[i].inflight)
			if(tx[tx_id].write_filter.intersect(tx[i].read_filter))
			{
				tx[i].status = INVALID;
			}
	}

}

void SpecSW_tx_post_commit()
{
	invalidate();
	pthread_mutex_unlock(&commit_lock);
}
