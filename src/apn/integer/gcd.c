/* GCD, Extended GCD
 *
 * Copyright (C) 2015 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/apn/integer.h>
#include <common/lib.h>
#include "integer.h"

#include "string.h"

/* The oldest algorithm */
static _tt_word gcd_naive(_tt_word a, _tt_word b)
{
	while (b) {
		_tt_word r = a % b;
		a = b;
		b = r;
	}

	return a;
}

/* Count trailing zero, a != 0 */
static uint __ctz(const _tt_word *a)
{
	uint z = 0;

	while (*a == 0) {
		a++;
		z += _tt_word_bits;
	}

	return z + _tt_int_word_ctz(*a);
}

/* Right shift to remove all trailing zeros */
static _tt_word *to_odd(_tt_word *a, int *msb, uint *shift)
{
	*shift = 0;

	while (*a == 0) {
		a++;
		(*msb)--;
		(*shift) += _tt_word_bits;
	}

	int zc = _tt_int_word_ctz(*a);
	if (zc) {
		*msb = _tt_int_shift_buf(a, *msb, -zc);
		(*shift) += zc;
	}

	return a;
}

static int copy_abs(struct tt_int *dst, const struct tt_int *src)
{
	if (!dst)
		return 0;

	int ret = _tt_int_copy(dst, src);
	dst->sign = 0;

	return ret;
}

/* Binary GCD */
static int gcd_binary(struct tt_int *g, _tt_word *a, int msba,
		_tt_word *b, int msbb)
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

	_tt_int_zero(g);
	int ret = _tt_int_realloc(g,
			msba + (shift+_tt_word_bits-1)/_tt_word_bits);
	if (ret)
		return ret;

	const uint sh_words = shift / _tt_word_bits;
	const uint sh_bits = shift % _tt_word_bits;
	memcpy(g->buf+sh_words, a, msba*_tt_word_sz);
	g->msb = _tt_int_shift_buf(g->buf+sh_words, msba, sh_bits) + sh_words;

	return 0;
}

/* Extended Binary GCD */
static int extgcd_binary(struct tt_int *g, struct tt_int *u, struct tt_int *v,
		_tt_word *a, int msba, _tt_word *b, int msbb)
{
	struct tt_int *u0 = u, *v0 = v;

	/* w = 0, x = 1 */
	struct tt_int *w = tt_int_alloc();
	struct tt_int *x = tt_int_alloc();
	tt_int_from_uint(x, 1);

	const uint shift = _tt_min(__ctz(a), __ctz(b));
	if (shift) {
		/* Remove common trailing zeros */
		msba = _tt_int_shift_buf(a, msba, -shift);
		msbb = _tt_int_shift_buf(b, msbb, -shift);
	}

	/* Make a odd */
	int swapped = 0;
	if ((a[0] & 0x1) == 0) {
		__tt_swap(a, b);
		__tt_swap(msba, msbb);
		swapped = 1;
	}

	/* Save a0, b0 */
	void *buf = malloc((msba+msbb)*_tt_word_sz);
	if (!buf)
		return TT_ENOMEM;
	_tt_word *a0 = buf;
	_tt_word *b0 = a0 + msba;
	memcpy(a0, a, msba*_tt_word_sz);
	memcpy(b0, b, msbb*_tt_word_sz);
	struct tt_int a0i = _TT_INT_DECL(msba, a0);
	struct tt_int b0i = _TT_INT_DECL(msbb, b0);

	do {
		uint tmpsh;
		b = to_odd(b, &msbb, &tmpsh);
		while (tmpsh--) {
			if (x->buf[0] & 0x1) {
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
	if (swapped)
		__tt_swap(*u0, *v0);

	/* GCD */
	if (g) {
		_tt_int_zero(g);
		int ret = _tt_int_realloc(g, msba +
				(shift+_tt_word_bits-1)/_tt_word_bits);
		if (ret)
			return ret;

		const uint sh_words = shift / _tt_word_bits;
		const uint sh_bits = shift % _tt_word_bits;
		memcpy(g->buf+sh_words, a, msba*_tt_word_sz);
		g->msb = _tt_int_shift_buf(g->buf+sh_words, msba, sh_bits) +
			sh_words;
	}

	return 0;
}

/* g = gcd(a, b) */
int tt_int_gcd(struct tt_int *g, const struct tt_int *a, const struct tt_int *b)
{
	if (_tt_int_is_zero(a))
		return copy_abs(g, b);
	else if (_tt_int_is_zero(b))
		return copy_abs(g, a);

	/* Copy int buffer */
	void *buf = malloc((a->msb+b->msb)*_tt_word_sz);
	if (!buf)
		return TT_ENOMEM;
	_tt_word *_a = buf;
	_tt_word *_b = _a + a->msb;
	memcpy(_a, a->buf, a->msb*_tt_word_sz);
	memcpy(_b, b->buf, b->msb*_tt_word_sz);

	int ret = gcd_binary(g, _a, a->msb, _b, b->msb);

	free(buf);
	return ret;
}

/* g = gcd(a,b) = ua + vb
 * - g may be NULL (for modular inverse)
 */
int tt_int_extgcd(struct tt_int *g, struct tt_int *u, struct tt_int *v,
		const struct tt_int *a, const struct tt_int *b)
{
	/* u = 1, v = 0 */
	tt_int_from_uint(u, 1);
	_tt_int_zero(v);

	if (_tt_int_is_zero(a)) {
		__tt_swap(*u, *v);
		return copy_abs(g, b);
	} else if (_tt_int_is_zero(b)) {
		return copy_abs(g, a);
	}

	/* Copy int buffer */
	void *buf = malloc((a->msb+b->msb)*_tt_word_sz);
	if (!buf)
		return TT_ENOMEM;
	_tt_word *_a = buf;
	_tt_word *_b = _a + a->msb;
	memcpy(_a, a->buf, a->msb*_tt_word_sz);
	memcpy(_b, b->buf, b->msb*_tt_word_sz);

	int ret = extgcd_binary(g, u, v, _a, a->msb, _b, b->msb);

	/* Adjust coefficient sign */
	if (a->sign)
		u->sign ^= 1;
	if (b->sign)
		v->sign ^= 1;

	free(buf);
	return ret;
}
