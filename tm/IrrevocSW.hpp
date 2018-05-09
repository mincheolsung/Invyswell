#ifndef __IRREVOCSW_HPP__
#define __IRREVOCSW_HPP__

#include "test_threads.hpp"
#include "SpecSW.hpp"

FORCE_INLINE void IrrevocSW_tx_begin(void)
{
	tx[tx_id].write_filter.clear();
	tx[tx_id].read_filter.clear();

	if (GET_VERSION(commit_lock) != (tx_id + 1))
		while (!TRY_LOCK(commit_lock)){}

	SET_VERSION(commit_lock, tx_id+1);
}

FORCE_INLINE void IrrevocSW_tx_read(uint64_t* addr)
{
	tx[tx_id].read_filter.add(addr);
}

FORCE_INLINE void IrrevocSW_tx_write(uint64_t* addr)
{
	tx[tx_id].write_filter.add(addr);
}

FORCE_INLINE void IrrevocSW_tx_end(void)
{
}

FORCE_INLINE void IrrevocSW_tx_post_commit(void)
{
	if (!tx[tx_id].write_filter.readonly()) // !read-only
		invalidate();

	UNLOCK(commit_lock);
}

#endif
