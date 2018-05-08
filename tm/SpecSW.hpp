#ifndef __SPECSW_HPP__
#define __SPECSW_HPP__

#include "test_threads.hpp"

#define  SpecSW_TX_BEGIN								\
{											\
	tx[tx_id].priority = 0;								\
	tx[tx_id].inflight = true;							\
	_setjmp(tx[tx_id].scope);							\
	tx[tx_id].priority++;								\
	__sync_fetch_and_add(&sw_cnt, 1);						\
	do										\
	{										\
		tx[tx_id].local_cs = commit_sequence;					\
	}										\
	while(tx[tx_id].local_cs & 1);							\
}						

FORCE_INLINE void validate(void)
{
	if(tx[tx_id].local_cs != commit_sequence)
		longjmp(tx[tx_id].scope, 1); // restart
	
	if(commit_lock)
	{
		if (GET_VERSION(commit_lock) != tx_id)
			longjmp(tx[tx_id].scope, 1); // restart
	}

	while(hw_post_commit != 0);

	if(tx[tx_id].status == INVALID)
		longjmp(tx[tx_id].scope, 1); // restart
}

FORCE_INLINE uint64_t SpecSW_tx_read(uint64_t* addr)
{
	WriteSetEntry log((void**)addr);
    	bool found = tx[tx_id].write_set->find(log);
    	if (__builtin_expect(found, true))
		return log.val;	

	tx[tx_id].read_filter.add(addr);
	uint64_t val = *addr;
	validate();
	return val;
} 

FORCE_INLINE void SpecSW_tx_write(uint64_t* addr, uint64_t val)
{
	if(tx[tx_id].status == INVALID)
		longjmp(tx[tx_id].scope, 1); // restart

	tx[tx_id].write_filter.add(addr);
	//add addr, val to local hash table
	tx->write_set->insert(WriteSetEntry((void**)addr, val));
}

FORCE_INLINE bool iBalance(struct Tx_Context *commitTx, struct Tx_Context *conflicts, int conflicts_size)
{
	Tx_Context *c = commitTx, *tx = NULL;
	int setSize = (c->write_set->size() * (c->priority + WC)) + (c->read_set->size() * (c->priority + RC));

	int abortPrio = 0, abortSetSize = 0, highestPrio = 0;
	bool mostReads = true, mostWrites = true;
	bool /*fewestCommits = true, */canAbort = false;

	for(int i=0; i<conflicts_size; i++)
	{
		tx = &conflicts[i];
		if(tx == NULL)
			break;

/*		if(tx->commits < c->commits)
			fewestCommits = false;*/
		if(tx->read_set->size() > c->read_set->size())
			mostReads = false;

		if(tx->write_set->size() > c->write_set->size())
                        mostWrites = false;

		if(tx->priority > highestPrio)
			highestPrio = tx->priority;

		abortSetSize += tx->write_set->size() + tx->read_set->size();
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

FORCE_INLINE bool CM_can_commit()
{
	struct Tx_Context conflicts[300];
	int conflicts_size = 0;

	//compare write set with read set of in-flight transactions and make set named conflicts
	for(int i=0; i<total_threads; i++)
	{ 
		if(tx[i].inflight)
			if(tx[tx_id].write_filter.intersect(&tx[i].read_filter))
			{
				conflicts[conflicts_size] =  tx[i];
				conflicts_size++;
			}
	}

	return iBalance(&(tx[tx_id]), conflicts, conflicts_size);
}

FORCE_INLINE void commit(void)
{
	tx[tx_id].write_set->writeback();
}

FORCE_INLINE void SpecSW_tx_end(void)
{
	if(tx[tx_id].write_set->size() == 0) //read-only
	{
		 __sync_fetch_and_sub(&sw_cnt, 1);	
		return;
	}

	while (!TRY_LOCK(commit_lock))
	{
		printf("try to lock\n");
	}
	SET_VERSION(commit_lock, tx_id);

	validate();
	if(!CM_can_commit())
		longjmp(tx[tx_id].scope, 1); // restart

	__sync_fetch_and_sub(&sw_cnt, 1);
	commit();
}

FORCE_INLINE void invalidate(void)
{
	//compare write set with read set of in-flight transactions and invalidate if match
	for(int i=0; i<total_threads; i++)
	{ 
		if(tx[i].inflight)
			if(tx[tx_id].write_filter.intersect(&tx[i].read_filter))
			{
				tx[i].status = INVALID;
			}
	}

}

FORCE_INLINE void SpecSW_tx_post_commit()
{
	invalidate();
	UNLOCK(commit_lock);
}

#endif
