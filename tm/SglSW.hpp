#ifndef __SGLSW_HPP__
#define __SGLSW_HPP__

#include "test_threads.hpp"

FORCE_INLINE void SglSW_tx_begin(void)
{
	pthread_mutex_lock(&commit_lock);	
	++commit_sequence;
}

FORCE_INLINE void SglSW_tx_end(void)
{
	++commit_sequence;
	pthread_mutex_unlock(&commit_lock);
}

#endif
