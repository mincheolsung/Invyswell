#ifndef __LIGHTHW__
#define __LIGHTHW__
#include "test_threads.hpp"

FORCE_INLINE void LightHW_tx_end(void)
{
	if (!IS_LOCKED(commit_lock) && !sw_cnt)
		_xend();
	else 
		_xabort(1);
}

#endif
