#include "test_threads.hpp"

#include "SpecSW.hpp"
#include "IrrevocSW.hpp"
#include "SglSW.hpp"
#include "LightHW.hpp"
#include "BFHW.hpp"

#define INVYSWELL_TX_BEGIN							  	\
	uint32_t abort_flags = _setjmp(tx[tx_id].scope);  	\
	unsigned int status;							 	\
														\
	switch(tx[tx_id].type) {							\
	/* LightHW */										\
	case 0:												\
		status = _xbegin();								\
		break;											\
	/* BFHW */											\
	case 1:												\
		status = _xbegin();								\
		break;											\
	/* SpecSW */										\
	case 2:												\
		if (tx[tx_id].attempts == 0)					\
		{												\
			status = 1;									\
			break;										\
		}												\
		tx[tx_id].attempts--;							\
		SpecSW_TX_BEGIN									\
		status = 0;										\
		break;											\
	/* SglSW */											\
	case 3:												\
		if (tx[tx_id].attempts == 0)					\
		{												\
			status = 1;									\
			break;										\
		}												\
		SglSW_tx_begin();								\
		status = 0;										\
		break;											\
	/* IrrevocSW */										\
	case 4:												\
		if (tx[tx_id].attempts == 0)					\
		{												\
			status = 1;									\
			break;										\
		}												\
		IrrevocSW_tx_begin();							\
		status = 0;										\
		break;											\
	default:											\
		status = 1;										\
		break;											\
	}

FORCE_INLINE uint64_t invyswell_tx_read(uint64_t *addr)
{
	uint64_t value;

	switch(tx[tx_id].type) {

	/* LightHW */
	case 0:
		value = *addr;	
		break;

	/* BFHW */
	case 1:
		BFHW_tx_read(addr);
		value = *addr;	
		break;

	/* SpecSW */
	case 2:
		value = SpecSW_tx_read(addr);
		break;

	/* SglSW */
	case 3:
		value = *addr;
		break;

	/* IrrevocSW */
	case 4:
		IrrevocSW_tx_read(addr);
		value = *addr;
		break;

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
		break;

	/* BFHW */
	case 1:
		BFHW_tx_write(addr);
		*addr = value;	
		break;

	/* SpecSW */
	case 2:
		SpecSW_tx_write(addr, value);
		break;

	/* SglSW */
	case 3:
		*addr = value;
		break;

	/* IrrevocSW */
	case 4:
		IrrevocSW_tx_write(addr);
		*addr = value;
		break;

	default:
		break;
	}
}

FORCE_INLINE void invyswell_tx_end(void)
{
	switch(tx[tx_id].type) {

	/* LightHW */
	case 0:
		LightHW_tx_end();	
		break;

	/* BFHW */
	case 1:
		BFHW_tx_end();
		BFHW_tx_post_commit();
		break;

	/* SpecSW */
	case 2:
		SpecSW_tx_end();
		SpecSW_tx_post_commit();
		break;

	/* SglSW */
	case 3:
		SglSW_tx_end();
		break;

	/* IrrevocSW */
	case 4:
		IrrevocSW_tx_end();
		IrrevocSW_tx_post_commit();
		break;

	default:
		break;
	}
}


