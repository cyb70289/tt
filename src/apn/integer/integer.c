/* Helpers of integer operations
 *
 * Copyright (C) 2015 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/apn/integer.h>
#include <common/lib.h>
#include "integer.h"

#include <string.h>

struct tt_int *tt_int_alloc(void)
{
	struct tt_int *ti = calloc(1, sizeof(struct tt_int));
	*(uint *)&ti->__sz = TT_INT_MAX + TT_INT_GUARD;
	*(uint *)&ti->_max = TT_INT_MAX;
	ti->_msb = 1;
	ti->_int = calloc(ti->__sz, 4);
	if (ti->_int == NULL) {
		tt_error("Not enough memory");
		free(ti);
		return NULL;
	}
	tt_debug("Integer created: %u bytes", ti->__sz * 4);

	return ti;
}

void tt_int_free(struct tt_int *ti)
{
	free(ti->_int);
	free(ti);
}

int _tt_int_realloc(struct tt_int *ti, uint msb)
{
	if (msb <= ti->_max)
		return 0;

	/* Increase half buffer */
	if (msb < ti->_max * 3 / 2)
		msb = ti->_max * 3 / 2;
	msb += TT_INT_GUARD;
	tt_assert(msb > ti->__sz);

	/* Reallocate buffer */
	uint *_int = realloc(ti->_int, msb * 4);
	if (!_int) {
		tt_error("Not enough memory");
		return TT_ENOMEM;
	}
	memset(_int + ti->__sz, 0, (msb - ti->__sz) * 4);
	ti->_int = _int;
	*(uint *)&ti->__sz = msb;
	*(uint *)&ti->_max = msb - TT_INT_GUARD;
	tt_debug("Integer reallocated: %u bytes", msb * 4);

	return 0;
}

void _tt_int_zero(struct tt_int *ti)
{
	ti->_sign = 0;
	ti->_msb = 1;
	memset(ti->_int, 0, ti->__sz * 4);
}

/* Check sanity */
int _tt_int_sanity(const struct tt_int *ti)
{
	if (ti->_msb == 0 || ti->_msb > ti->_max)
		return TT_APN_ESANITY;

	uint i;

	/* Check carry guard bit */
	for (i = 0; i < ti->_msb; i++)
		if ((ti->_int[i] & BIT(31)))
			return TT_APN_ESANITY;

	/* Check remaining uints */
	for (; i < ti->__sz; i++)
		if (ti->_int[i])
			return TT_APN_ESANITY;

	return 0;
}
