#include "test_threads.hpp"

#include "SpecSW.hpp"
#include "IrrevocSW.hpp"
#include "SglSW.hpp"
#include "LightHW.hpp"
#include "BFHW.hpp"

#define _STM_STARTED (1<<6)
#define _STM_STOPPED (1<<7)
#define _INVYSWELL_ERROR (1<<8)

#define INVYSWELL_TX_BEGIN							  	\
	uint32_t abort_flags = _setjmp(tx[tx_id].scope);  	\
	unsigned int status = _INVYSWELL_ERROR;				\
	switch(tx[tx_id].type) {							\
	/* LightHW */										\
	case 0:												\
		status = _xbegin();								\
		break;											\
	/* BFHW */											\
	case 1:												\
		tx[tx_id].write_filter.clear();					\
		tx[tx_id].read_filter.clear();					\
		status = _xbegin();								\
		break;											\
	/* SpecSW */										\
	case 2:												\
		if (tx[tx_id].attempts == 0)					\
		{												\
			status = _STM_STOPPED;						\
			break;										\
		}												\
		tx[tx_id].attempts--;							\
		SpecSW_TX_BEGIN									\
		status = _STM_STARTED;							\
		break;											\
	/* IrrevocSW */										\
	case 3:												\
		if (tx[tx_id].attempts == 0)					\
		{												\
			status = _STM_STOPPED;						\
			break;										\
		}												\
		IrrevocSW_tx_begin();							\
		status = _STM_STARTED;							\
		break;											\
	/* SglSW */											\
	case 5:												\
		if (tx[tx_id].attempts == 0)					\
		{												\
			status = _STM_STOPPED;						\
			break;										\
		}												\
		SglSW_tx_begin();								\
		status = _STM_STARTED;							\
		break;											\
	default:											\
		status = _INVYSWELL_ERROR;						\
		break;											\
	}

FORCE_INLINE uint64_t invyswell_tx_read(uint64_t *addr)
{
	uint64_t value;

	switch(tx[tx_id].type) {

	/* LightHW */
	case 0:
		value = *addr;
		return value;

	/* BFHW */
	case 1:
		BFHW_tx_read(addr);
		value = *addr;
		return value;

	/* SpecSW */
	case 2:
		value = SpecSW_tx_read(addr);
		return value;

	/* IrrevocSW */
	case 3:
		IrrevocSW_tx_read(addr);
		value = *addr;
		return value;

	/* SglSW */
	case 5:
		value = *addr;
		return value;


	default:
		break;
	}

	return value;
}

FORCE_INLINE void invyswell_tx_write(uint64_t *addr, uint64_t value)
{
	switch(tx[tx_id].type) {

	/* LightHW */
	case 0:
		*addr = value;
		return;

	/* BFHW */
	case 1:
		BFHW_tx_write(addr);
		*addr = value;
		return;

	/* SpecSW */
	case 2:
		SpecSW_tx_write(addr, value);
		return;

	/* IrrevocSW */
	case 3:
		IrrevocSW_tx_write(addr);
		*addr = value;
		return;

	/* SglSW */
	case 5:
		*addr = value;
		return;

	default:
		return;
	}
}

FORCE_INLINE void invyswell_tx_end(void)
{
	switch(tx[tx_id].type) {

	/* LightHW */
	case 0:
		LightHW_tx_end();	
		return;

	/* BFHW */
	case 1:
		BFHW_tx_end();
		BFHW_tx_post_commit();
		return;

	/* SpecSW */
	case 2:
		SpecSW_tx_end();
		SpecSW_tx_post_commit();
		return;

	/* IrrevocSW */
	case 3:
		IrrevocSW_tx_end();
		IrrevocSW_tx_post_commit();
		return;

	/* SglSW */
	case 5:
		SglSW_tx_end();
		return;

	default:
		return;
	}
}
