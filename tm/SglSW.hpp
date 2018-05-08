#ifndef __SGLSW_HPP__
#define __SGLSW_HPP__

#include "test_threads.hpp"

FORCE_INLINE void SglSW_tx_begin(void)
{
	while(!TRY_LOCK(commit_lock))
	{}
	SET_VERSION(commit_lock, tx_id);
	++commit_sequence;
}

FORCE_INLINE void SglSW_tx_end(void)
{
	++commit_sequence;
	UNLOCK(commit_lock);
}

#endif
