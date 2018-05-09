#ifndef __SPECSW_HPP__
#define __SPECSW_HPP__

#include "test_threads.hpp"

#define  SpecSW_TX_BEGIN									\
{															\
	tx[tx_id].priority = 0;									\
	tx[tx_id].inflight = true;								\
	tx[tx_id].priority++;									\
	tx[tx_id].status = VALID;								\
	tx[tx_id].write_set->reset();							\
	tx[tx_id].read_set->reset();							\
	tx[tx_id].write_filter.clear();							\
	tx[tx_id].read_filter.clear();							\
	__sync_fetch_and_add(&sw_cnt, 1);						\
	do														\
	{														\
		tx[tx_id].local_cs = commit_sequence;				\
	}														\
	while(tx[tx_id].local_cs & 1);							\
}						

FORCE_INLINE void validate(void)
{
	if(tx[tx_id].local_cs != commit_sequence)
		goto jmp;
	
	if(IS_LOCKED(commit_lock))
	{
		if (GET_VERSION(commit_lock) != (tx_id+1))
		{
			int other = GET_VERSION(commit_lock) - 1;
			if (tx[tx_id].read_filter.intersect(&tx[other].write_filter))
				goto jmp;
		}
	}

	while(hw_post_commit != 0){}

	if(tx[tx_id].status == INVALID)
		goto jmp;

	return;

jmp:
	tx[tx_id].racy_shared++;
	if (tx[tx_id].racy_shared > RACY_THRESHOLD)
		tx[tx_id].fail_fast = 1;

	longjmp(tx[tx_id].scope, 1); // restart
	return;
}

FORCE_INLINE uint64_t SpecSW_tx_read(uint64_t* addr)
{
	WriteSetEntry log((void**)addr);
    bool found = tx[tx_id].write_set->find(log);
	if (__builtin_expect(found, false))
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
	tx[tx_id].write_set->insert(WriteSetEntry((void**)addr, *((uint64_t*)(&val))));
}

FORCE_INLINE bool iBalance(struct Tx_Context *commitTx, struct Tx_Context *conflicts, int conflicts_size)
{
	Tx_Context *c = commitTx, *tx = NULL;
	int setSize = (c->write_set->size() * (c->priority + WC)) + (c->read_set->size() * (c->priority + RC));

	int abortPrio = 0, abortSetSize = 0, highestPrio = 0;
	bool mostReads = true, mostWrites = true;
	bool canAbort = false;

	for(int i=0; i<conflicts_size; i++)
	{
		tx = &conflicts[i];
		if(tx == NULL)
			break;
		
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

	if(p >= highestPrio)
		canAbort = true;

	if(mostReads || mostWrites)
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
		{
			if(tx[tx_id].write_filter.intersect(&tx[i].read_filter))
			{
				conflicts[conflicts_size] =  tx[i];
				conflicts_size++;
			}
		}
	}

	return iBalance(&(tx[tx_id]), conflicts, conflicts_size);
}

FORCE_INLINE void commit(void)
{
	tx[tx_id].write_set->writeback();
	CFENCE;
}

FORCE_INLINE void SpecSW_tx_end(void)
{
	if(tx[tx_id].write_set->size() == 0) //read-only
	{
		 __sync_fetch_and_sub(&sw_cnt, 1);	
		return;
	}
	
	if (GET_VERSION(commit_lock) != (tx_id + 1))
		while (!TRY_LOCK(commit_lock)){}
	SET_VERSION(commit_lock, tx_id+1);

	validate();

	if(!CM_can_commit())
		longjmp(tx[tx_id].scope, 1); // restart

	__sync_fetch_and_sub(&sw_cnt, 1);
	commit();
}

FORCE_INLINE void invalidate(void)
{
	for(int i=0; i<total_threads; i++)
	{
		if (i == tx_id)
			continue;

		if(tx[i].inflight)
		{
			if(tx[tx_id].write_filter.intersect(&tx[i].read_filter))
			{
				tx[i].status = INVALID;
			}
		}
	}
}

FORCE_INLINE void SpecSW_tx_post_commit()
{
	invalidate();
	UNLOCK(commit_lock);
}

#endif
