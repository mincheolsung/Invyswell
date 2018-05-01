#ifndef __SGLSW_HPP__
#define __SGLSW_HPP__

#include "invyswell.h"

void SglSW_tx_begin(void)
{
	pthread_mutex_lock(&commit_lock);	
	++commit_sequence;
}

void SglSW_tx_end(void)
{
	++commit_sequence;
	pthread_mutex_unlock(&commit_lock);
}

#endif
