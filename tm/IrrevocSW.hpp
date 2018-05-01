#ifndef __IRREVOCSW_HPP__
#define __IRREVOCSW_HPP__

#include "invyswell.h"
#include "SpecSW.hpp"

FORCE_INLINE void IrrevocSW_tx_begin(void)
{
	pthread_mutex_lock(&commit_lock);	
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
	if(tx[tx_id].writeset->size() != 0) //read-only
		invalidate();

	pthread_mutex_unlock(&commit_lock);
}

#endif
