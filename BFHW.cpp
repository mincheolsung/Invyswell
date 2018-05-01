#include "invyswell.h"

void BFHW_tx_read(Tx_Context *tx, uint64_t *addr)
{
	tx->read_filter.add(addr);
}

void BFHW_tx_write(Tx_Context *tx, uint64_t *addr)
{
	tx->write_filter.add(addr);
}

void BFHW_tx_end(Tx_Context *tx)
{
	if (!commit_lock)
	{
		++hw_post_commit;
		_xend();
	}
	else 
	{
		for (int id = 0; id < NUM_OF_THREAD; id++ )
		{
			if (id == tx->id)
				continue;

			if (tx->write_filter.intersect(&global_tx[id]->write_filter))
				_xabort();

			if (tx->read_filter.intersect(&global_tx[id]->read_filter))
				_xabort();
		}

		++hw_post_commit;
		_xend();
	}
}

void BFHW_tx_post_commit(void)
{

}
