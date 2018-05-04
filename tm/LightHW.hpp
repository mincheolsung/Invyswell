#ifndef __LIGHTHW__
#define __LIGHTHW__
#include "test_threads.hpp"

FORCE_INLINE void LightHW_tx_end(void)
{
	if ((pthread_mutex_trylock(&commit_lock) == 0) && (sw_cnt == 0))
	{
		pthread_mutex_unlock(&commit_lock);
		_xend();
	}
	else 
		_xabort(1);
}

#endif
