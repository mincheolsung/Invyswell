#ifndef __BFHW__
#define __BFHW__

#include "test_threads.hpp"

FORCE_INLINE void BFHW_tx_read(uint64_t *addr)
{
	tx[tx_id].read_filter.add(addr);
}

FORCE_INLINE void BFHW_tx_write(uint64_t *addr)
{
	tx[tx_id].write_filter.add(addr);
}

FORCE_INLINE void BFHW_tx_end(void)
{
	if (pthread_mutex_trylock(&commit_lock) == 0)
	{
		pthread_mutex_unlock(&commit_lock);
		++hw_post_commit;
		_xend();
	}
	else
	{
		for (int id = 0; id < total_threads; id++ )
		{
			/*
			if (id == tx_id)
				continue;
			*/
			if (tx[id].inflight)
				if (tx[tx_id].write_filter.intersect(&tx[id].read_filter))
					_xabort(1);
		}
		++hw_post_commit;
		_xend();
	}
}

FORCE_INLINE void BFHW_tx_post_commit(void)
{
	if(tx[tx_id].write_set->size() != 0) // !read-only
		invalidate();

	__sync_fetch_and_sub(&hw_post_commit, 1);
}
#endif
