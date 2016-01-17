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

void _tt_int_swap(struct tt_int *ti1, struct tt_int *ti2)
{
	struct tt_int ti = *ti1;
	memcpy(ti1, ti2, sizeof(struct tt_int));
	memcpy(ti2, &ti, sizeof(struct tt_int));
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

void _tt_int_print(const struct tt_int *ti)
{
	char *s = NULL;
	tt_int_to_string(ti, &s, 10);
	printf("%s\n", s);
	free(s);
}

void _tt_int_print_buf(const uint *ui, int msb)
{
	struct tt_int ti = _TT_INT_DECL(msb, ui);
	_tt_int_print(&ti);
}

/* Shift integer
 * - shift: + left, - right
 * - _int buffer must be large enough
 * - return new msb
 * - fill empty space with zero
 */
int _tt_int_shift_buf(uint *_int, int msb, int shift)
{
	int sh_uints, sh_bits;
	uint tmp = 0, tmp2;

	if (shift > 0) {
		sh_uints = shift / 31;
		sh_bits = shift % 31;

		if (sh_uints) {
			memmove(_int+sh_uints, _int, msb*4);
			memset(_int, 0, sh_uints*4);
			msb += sh_uints;
		}

		if (sh_bits) {
			for (int i = sh_uints; i < msb; i++) {
				tmp2 = _int[i];
				_int[i] = ((_int[i] << sh_bits) | tmp) & ~BIT(31);
				tmp = tmp2 >> (31 - sh_bits);
			}
			if (tmp)
				_int[msb++] = tmp;
		}
	} else if (shift < 0) {
		shift = -shift;
		sh_uints = shift / 31;
		sh_bits = shift % 31;

		if (sh_uints >= msb) {
			memset(_int, 0, msb*4);
			return 1;
		}

		if (sh_uints) {
			memmove(_int, _int+sh_uints, (msb-sh_uints)*4);
			memset(_int+msb-sh_uints, 0, sh_uints*4);
			msb -= sh_uints;
		}

		if (sh_bits) {
			for (int i = msb-1; i >= 0; i--) {
				tmp2 = _int[i];
				_int[i] = (_int[i] >> sh_bits) | tmp;
				tmp = (tmp2 << (31 - sh_bits)) & ~BIT(31);
			}
			if (_int[msb-1] == 0 && msb > 1)
				msb--;
		}
	}

	return msb;
}

/* Shift integer: shift: + left, - right */
int tt_int_shift(struct tt_int *ti, int shift)
{
	if (shift > 0) {
		int ret = _tt_int_realloc(ti, ti->_msb + (shift+30)/31);
		if (ret)
			return ret;
	}

	ti->_msb = _tt_int_shift_buf(ti->_int, ti->_msb, shift);
	return 0;
}
