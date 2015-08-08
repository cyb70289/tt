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
	ti->_cnt = TT_INT_CNT;
	ti->_msb = 1;
	ti->_int = calloc(ti->_cnt, 4);
	if (ti->_int == NULL) {
		tt_error("Not enough memory");
		free(ti);
		return NULL;
	}
	tt_debug("Integer created: %u bytes", ti->_cnt * 4);

	return ti;
}

void tt_int_free(struct tt_int *ti)
{
	free(ti->_int);
	free(ti);
}

void _tt_int_zero(struct tt_int *ti)
{
	ti->_sign = 0;
	ti->_msb = 1;
	memset(ti->_int, 0, ti->_cnt * 4);
}

/* Check sanity */
int _tt_int_sanity(const struct tt_int *ti)
{
	if (ti->_msb == 0 || ti->_msb > ti->_cnt)
		return TT_APN_ESANITY;

	int i;

	/* Check carry guard bit */
	for (i = 0; i < ti->_msb; i++)
		if ((ti->_int[i] & BIT(31)))
			return TT_APN_ESANITY;

	/* Check remaining uints */
	for (; i < ti->_cnt; i++)
		if (ti->_int[i])
			return TT_APN_ESANITY;

	return 0;
}
