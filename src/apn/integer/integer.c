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
	*(uint *)&ti->_max = TT_INT_DEF;
	ti->_msb = 1;
	ti->_int = calloc(ti->_max, 4);
	if (ti->_int == NULL) {
		tt_error("Out of memory");
		free(ti);
		return NULL;
	}
	tt_debug("Integer created: %u bytes", ti->_max * 4);

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
	tt_assert(msb > ti->_max);

	/* Reallocate buffer */
	uint *_int = realloc(ti->_int, msb * 4);
	if (!_int)
		return TT_ENOMEM;
	memset(_int + ti->_max, 0, (msb - ti->_max) * 4);
	ti->_int = _int;
	*(uint *)&ti->_max = msb;
	tt_debug("Integer reallocated: %u bytes", msb * 4);

	return 0;
}

int _tt_int_copy(struct tt_int *dst, const struct tt_int *src)
{
	_tt_int_zero(dst);

	int ret = _tt_int_realloc(dst, src->_msb);
	if (ret)
		return ret;

	dst->_sign = src->_sign;
	dst->_msb = src->_msb;
	memcpy(dst->_int, src->_int, src->_msb * 4);

	return 0;
}

void _tt_int_zero(struct tt_int *ti)
{
	ti->_sign = 0;
	ti->_msb = 1;
	memset(ti->_int, 0, ti->_max * 4);
}

/* Check sanity */
int _tt_int_sanity(const struct tt_int *ti)
{
	uint i;

	if (ti->_msb == 0 || ti->_msb > ti->_max)
		return TT_APN_ESANITY;

	/* Check significands */
	if (ti->_msb > 1 && ti->_int[ti->_msb-1] == 0)
		return TT_APN_ESANITY;

	/* Check carry guard bit */
	for (i = 0; i < ti->_msb; i++)
		if ((ti->_int[i] & BIT(31)))
			return TT_APN_ESANITY;

	/* Check unused uints */
	for (; i < ti->_max; i++)
		if (ti->_int[i])
			return TT_APN_ESANITY;

	return 0;
}

/* Shift integer
 * - shift: + left, - right
 * - return new msb
 * - shift should be in [-31, 31]
 */
int _tt_int_shift_buf(uint *_int, int msb, int shift)
{
	tt_assert_fa(shift >= -31 && shift <= 31);

	uint tmp = 0, tmp2;

	if (shift > 0) {
		for (int i = 0; i < msb; i++) {
			tmp2 = _int[i];
			_int[i] = ((_int[i] << shift) | tmp) & ~BIT(31);
			tmp = tmp2 >> (31 - shift);
		}
		if (tmp)
			_int[msb++] = tmp;
	} else if (shift < 0) {
		shift = -shift;
		for (int i = msb-1; i >= 0; i--) {
			tmp2 = _int[i];
			_int[i] = (_int[i] >> shift) | tmp;
			tmp = (tmp2 << (31 - shift)) & ~BIT(31);
		}
		if (_int[msb-1] == 0 && msb > 1)
			msb--;
	}

	return msb;
}
