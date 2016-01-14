/* GCD, Extended GCD
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

/* Count trailing zero, a != 0 */
static uint __ctz(const uint *a)
{
	uint z = 0;

	while (*a == 0) {
		a++;
		z += 31;
	}

	return z + __builtin_ctz(*a);
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

/* Binary GCD */
static int gcd_binary(struct tt_int *g, uint *a, int msba, uint *b, int msbb)
{
	if (msba == 1 && msbb == 1) {
		tt_int_from_uint(g, gcd_naive(*a, *b));
		return 0;
	}

	uint shifta, shiftb;
	a = to_odd(a, &msba, &shifta);
	b = to_odd(b, &msbb, &shiftb);
	const uint shift = _tt_min(shifta, shiftb);

	int cmp;
	while ((cmp = _tt_int_cmp_buf(a, msba, b, msbb))) {
		if (cmp < 0) {
			__tt_swap(a, b);
			__tt_swap(msba, msbb);
		}
		msba = _tt_int_sub_buf(a, msba, b, msbb);
		a = to_odd(a, &msba, &shifta);
	}

	int ret = _tt_int_realloc(g, msba + (shift+30)/31);
	if (ret)
		return ret;

	const uint sh_uints = shift / 31;
	const uint sh_bits = shift % 31;
	memcpy(g->_int+sh_uints, a, msba*4);
	g->_msb = _tt_int_shift_buf(g->_int+sh_uints, msba, sh_bits) + sh_uints;

	return 0;
}

/* Extended Binary GCD */
static int extgcd_binary(struct tt_int *g, struct tt_int *u, struct tt_int *v,
		uint *a, int msba, uint *b, int msbb)
{
	struct tt_int *u0 = u, *v0 = v;

	const uint shifta = __ctz(a);
	const uint shiftb = __ctz(b);
	const uint shift = _tt_min(shifta, shiftb);

	if (shift) {
		/* Remove common zero */
		msba = _tt_int_shift_buf(a, msba, -shift);
		msbb = _tt_int_shift_buf(b, msbb, -shift);

		/* Make b odd */
		if (shiftb > shifta) {
			__tt_swap(a, b);
			__tt_swap(msba, msbb);
		}
	}

	/* Save a0, b0 */
	void *buf = malloc((msba+msbb)*4);
	if (!buf)
		return TT_ENOMEM;
	uint *a0 = buf;
	uint *b0 = a0 + msba;
	memcpy(a0, a, msba*4);
	memcpy(b0, b, msbb*4);

	const struct tt_int a0i = {
		._sign = 0, ._max = msba, ._msb = msba, ._int = a0 };
	const struct tt_int b0i = {
		._sign = 0, ._max = msbb, ._msb = msbb, ._int = b0 };

	/* w = 0, x = 1 */
	struct tt_int *w = tt_int_alloc();
	struct tt_int *x = tt_int_alloc();
	tt_int_from_uint(x, 1);

	do {
		uint tmpsh;
		b = to_odd(b, &msbb, &tmpsh);
		while (tmpsh--) {
			if ((w->_int[0] & 0x1)) {
				tt_int_add(w, w, &b0i);
				tt_int_sub(x, x, &a0i);
			}
			tt_int_shift(w, -1);
			tt_int_shift(x, -1);
		}

		if (_tt_int_cmp_buf(a, msba, b, msbb) > 0) {
			__tt_swap(a, b);
			__tt_swap(msba, msbb);
			__tt_swap(u, w);
			__tt_swap(v, x);
		}

		msbb = _tt_int_sub_buf(b, msbb, a, msba);
		tt_int_sub(w, w, u);
		tt_int_sub(x, x, v);
	} while (!(msbb == 1 && b[0] == 0));

	free(buf);

	/* Coefficients */
	if (u != u0) {
		_tt_int_copy(u0, u);
		_tt_int_copy(v0, v);
		tt_int_free(u);
		tt_int_free(v);
	} else {
		tt_int_free(w);
		tt_int_free(x);
	}
	if (shiftb > shifta)
		_tt_int_swap(u0, v0);

	/* GCD */
	int ret = _tt_int_realloc(g, msba + (shift+30)/31);
	if (ret)
		return ret;

	const uint sh_uints = shift / 31;
	const uint sh_bits = shift % 31;
	memcpy(g->_int+sh_uints, a, msba*4);
	g->_msb = _tt_int_shift_buf(g->_int+sh_uints, msba, sh_bits) + sh_uints;

	return 0;
}

/* g = gcd(a, b) */
int tt_int_gcd(struct tt_int *g, const struct tt_int *a, const struct tt_int *b)
{
	if (_tt_int_is_zero(a))
		return _tt_int_copy(g, b);
	else if (_tt_int_is_zero(b))
		return _tt_int_copy(g, a);

	/* Copy int buffer */
	void *buf = malloc((a->_msb + b->_msb) * 4);
	if (!buf)
		return TT_ENOMEM;
	uint *_a = buf;
	uint *_b = _a + a->_msb;
	memcpy(_a, a->_int, a->_msb*4);
	memcpy(_b, b->_int, b->_msb*4);

	_tt_int_zero(g);
	g->_sign = a->_sign ^ b->_sign;
	int ret = gcd_binary(g, _a, a->_msb, _b, b->_msb);

	free(buf);
	return ret;
}

/* g = gcd(a,b) = ua + vb */
int tt_int_extgcd(struct tt_int *g, struct tt_int *u, struct tt_int *v,
		const struct tt_int *a, const struct tt_int *b)
{
	/* u = 1, v = 0 */
	tt_int_from_uint(u, 1);
	_tt_int_zero(v);

	if (_tt_int_is_zero(a))
		return _tt_int_copy(g, b);
	else if (_tt_int_is_zero(b))
		return _tt_int_copy(g, a);

	/* Copy int buffer */
	void *buf = malloc((a->_msb + b->_msb) * 4);
	if (!buf)
		return TT_ENOMEM;
	uint *_a = buf;
	uint *_b = _a + a->_msb;
	memcpy(_a, a->_int, a->_msb*4);
	memcpy(_b, b->_int, b->_msb*4);

	_tt_int_zero(g);
	g->_sign = a->_sign ^ b->_sign;
	int ret = extgcd_binary(g, u, v, _a, a->_msb, _b, b->_msb);

	free(buf);
	return ret;
}
