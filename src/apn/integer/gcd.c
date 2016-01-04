/* Greatest Common Divisor
 *
 * Copyright (C) 2015 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/apn/integer.h>
#include <common/lib.h>
#include "integer.h"

#include "string.h"

static uint gcd_naive(uint a, uint b)
{
	while (b) {
		uint r = a % b;
		a = b;
		b = r;
	}

	return a;
}

/* Right shift to remove all trailing zeros */
static uint *to_odd(uint *a, int *msb, uint *shift)
{
	*shift = 0;

	while (*a == 0) {
		a++;
		(*msb)--;
		(*shift) += 31;
	}

	int zc = __builtin_ctz(*a);
	if (zc) {
		*msb = _tt_int_shift_buf(a, *msb, -zc);
		(*shift) += zc;
	}

	return a;
}

/* Division free GCD algorithm */
static int gcd_binary(uint *g, int *msb, uint *a, int msba, uint *b, int msbb)
{
	if (msba == 1 && msbb == 1) {
		*g = gcd_naive(*a, *b);
		*msb = 1;
		return 0;
	}

	uint shifta, shiftb;
	a = to_odd(a, &msba, &shifta);
	b = to_odd(b, &msbb, &shiftb);
	const uint shift = _tt_min(shifta, shiftb);

	int cmp;
	while ((cmp = _tt_int_cmp_buf(a, msba, b, msbb))) {
		if (cmp > 0) {
			msba = _tt_int_sub_buf(a, msba, b, msbb);
		} else {
			msbb = _tt_int_sub_buf(b, msbb, a, msba);
			__tt_swap(a, b);
			__tt_swap(msba, msbb);
		}
		a = to_odd(a, &msba, &shifta);
	}

	const uint sh_uints = shift / 31;
	const uint sh_bits = shift % 31;
	memcpy(g+sh_uints, a, msba*4);
	*msb = _tt_int_shift_buf(g+sh_uints, msba, sh_bits) + sh_uints;

	return 0;
}

/* g must be big enough */
int _tt_int_gcd_buf(uint *g, int *msb,
		const uint *a, int msba, const uint *b, int msbb)
{
	int ret;

	if (a[0] == 0 && msba == 1) {
		memcpy(g, b, msbb*4);
		*msb = msbb;
		return 0;
	} else if (b[0] == 0 && msbb == 1) {
		memcpy(g, a, msba*4);
		*msb = msba;
		return 0;
	}

	uint *buf = malloc((msba+msbb)*4);
	if (!buf)
		return TT_ENOMEM;
	uint *_a = buf;
	uint *_b = _a + msba;
	memcpy(_a, a, msba*4);
	memcpy(_b, b, msbb*4);

	ret = gcd_binary(g, msb, _a, msba, _b, msbb);

	free(buf);
	return ret;
}

int tt_int_gcd(struct tt_int *g, const struct tt_int *a, const struct tt_int *b)
{
	int ret, msb;

	/* TODO: realloc buffer only if necessary */
	if (_tt_int_is_zero(a))
		msb = b->_msb;
	else if (_tt_int_is_zero(b))
		msb = a->_msb;
	else
		msb = _tt_min(a->_msb, b->_msb);

	_tt_int_zero(g);
	ret = _tt_int_realloc(g, msb);
	if (ret)
		return ret;

	g->_sign = a->_sign ^ b->_sign;
	return _tt_int_gcd_buf(g->_int, &g->_msb,
			a->_int, a->_msb, b->_int, b->_msb);
}
