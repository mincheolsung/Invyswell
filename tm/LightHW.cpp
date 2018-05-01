#include "invyswell.h"

void LightHW_tx_end(void)
{
	if (pthread_mutex_trylock(&commit_lock) == 0 && !sw_cnt)
	{
		pthread_mutex_unlock(&commit_lock);
		_xend();
	}
	else 
		_xabort();
}
