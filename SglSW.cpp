#include "invyswell.h"

void SglSW_tx_begin(void)
{
	pthread_mutex_lock(&commit_lock);	
	++commit_sequence;
}

void IrrevocSW_tx_end(void)
{
	++commit_sequence;
	pthread_mutex_unlock(&commit_lock);
}
