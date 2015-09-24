/* Add, sub, mul, div, logic
 *
 * Copyright (C) 2015 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/apn/integer.h>
#include <common/lib.h>
#include "integer.h"

#include <string.h>

#define KARA_CROSS	200	/* Karatsuba/classic cross point */

/* Add two int and carry
 * - i1, i2 < 2^31
 * - return result uint and carry
 */
static uint add_int(uint i1, uint i2, int *carry)
{
	int r = i1 + i2 + *carry;

	if (r < 0) {
		r &= ~BIT(31);
		*carry = 1;
	} else {
		*carry = 0;
	}

	return r;
}

/* Sub two uint (31 valid bits) and borrow
 * - i1, i2 < 2^31
 * - return result uint and borrow
 */
static uint sub_int(uint i1, uint i2, int *borrow)
{
	int r = i1 - i2 - *borrow;

	if (r < 0) {
		r &= ~BIT(31);
		*borrow = 1;
	} else {
		*borrow = 0;
	}

	return r;
}

/* dst = |src1| + |src2|
 * - dst may share src1 or src2
 * - dst buffer may be smaller than required
 */
static int add_ints_abs(struct tt_int *dst, const struct tt_int *src1,
		const struct tt_int *src2)
{
	int ret;

	/* Make src1 longer than src2 */
	if (src1->_msb < src2->_msb)
		__tt_swap(src1, src2);

	/* Make sure dst buffer is large enough */
	if (dst->_max <= src1->_msb) {
		ret = _tt_int_realloc(dst, src1->_msb+1);
		if (ret)
			return ret;
	}
	dst->_msb = src1->_msb;

	/* Add uint array */
	int i, carry = 0;
	for (i = 0; i < src2->_msb; i++)
		dst->_int[i] = add_int(src1->_int[i], src2->_int[i], &carry);
	for (; i < src1->_msb; i++)
		dst->_int[i] = add_int(src1->_int[i], 0, &carry);
	if (carry) {
		dst->_int[i] = 1;
		dst->_msb++;
	}

	return 0;
}

/* dst = |src1| - |src2|
 * - |src1| > |src2|
 * - dst may share src1 or src2
 * - dst buffer may be smaller than required
 */
static int sub_ints_abs(struct tt_int *dst, const struct tt_int *src1,
		const struct tt_int *src2)
{
	/* Make sure dst buffer is large enough. TODO: It's overkill. */
	if (dst->_max < src1->_msb) {
		int ret = _tt_int_realloc(dst, src1->_msb);
		if (ret)
			return ret;
	}

	/* Sub uint array */
	int i, borrow = 0;
	for (i = 0; i < src2->_msb; i++)
		dst->_int[i] = sub_int(src1->_int[i], src2->_int[i], &borrow);
	for (; i < src1->_msb; i++)
		dst->_int[i] = sub_int(src1->_int[i], 0, &borrow);

	/* Check new msb */
	dst->_msb = src1->_msb;
	while (dst->_msb > 1 && dst->_int[dst->_msb-1] == 0)
		dst->_msb--;

	return 0;
}

/* dst = src1 +/- src2
 * - sub: 0 -> add, 1 -> sub
 * - dst may share src1 or src2
 */
static int add_sub_ints(struct tt_int *dst, const struct tt_int *src1,
		const struct tt_int *src2, int sub)
{
	int ret = 0;

	/* Zero dst if not overlap with src1 and src2 */
	if (dst != src1 && dst != src2)
		_tt_int_zero(dst);

	int sign1 = src1->_sign;
	int sign2 = src2->_sign;
	if (sub)
		sign2 = !sign2;

	if (sign1 == sign2) {
		dst->_sign = sign1;
		ret = add_ints_abs(dst, src1, src2);
	} else {
		int cmp12 = tt_int_cmp_abs(src1, src2);
		if (cmp12 >= 0) {
			dst->_sign = sign1;
			ret = sub_ints_abs(dst, src1, src2);
		} else {
			dst->_sign = sign2;
			ret = sub_ints_abs(dst, src2, src1);
		}
	}

	return ret;
}

/* int1 += int2
 * - int1 must have enough space to hold result
 * - return result msb
 */
static int add_buf(uint *int1, int msb1, const uint *int2, int msb2)
{
	int i, carry = 0;

	for (i = 0; i < msb2; i++)
		int1[i] = add_int(int1[i], int2[i], &carry);
	for (; i < msb1; i++) {
		if (carry == 0)
			return msb1;
		int1[i] = add_int(int1[i], 0, &carry);
	}
	if (carry)
		int1[i++] = 1;

	return i;
}

/* int1 -= int2
 * - int1 >= int2
 * - return result msb
 */
static int sub_buf(uint *int1, int msb1, const uint *int2, int msb2)
{
	int i, borrow = 0;

	for (i = 0; i < msb2; i++)
		int1[i] = sub_int(int1[i], int2[i], &borrow);
	for (; i < msb1; i++) {
		if (borrow == 0)
			return msb1;
		int1[i] = sub_int(int1[i], 0, &borrow);
	}
	while (i > 1 && int1[i-1] == 0)
		i--;

	return i;
}

/* 1: src1 > src2, 0: src1 == src2, -1: src1 < src2 */
static int cmp_buf(const uint *int1, int msb1, const uint *int2, int msb2)
{
	if (msb1 > msb2)
		return 1;
	else if (msb1 < msb2)
		return -1;

	for (int i = msb1-1; i >= 0; i--) {
		if (int1[i] > int2[i])
			return 1;
		else if (int1[i] < int2[i])
			return -1;
	}

	return 0;
}

#ifdef __SIZEOF_INT128__
static int from128(uint out[5], __uint128_t i128)
{
	int msb = 0;

	while (i128) {
		out[msb++] = i128 & (BIT(31) - 1);
		i128 >>= 31;
	}
	for (int i = msb; i < 5; i++)
		out[i] = 0;

	if (msb == 0)
		msb = 1;
	return msb;
}
#else
static int from64(uint out[3], uint64_t i64)
{
	int msb = 0;

	while (i64) {
		out[msb++] = i64 & (BIT(31) - 1);
		i64 >>= 31;
	}
	for (int i = msb; i < 3; i++)
		out[i] = 0;

	if (msb == 0)
		msb = 1;
	return msb;
}
#endif

static int get_uints(const uint *ui, int len)
{
	while (len) {
		if (ui[len-1])
			return len;
		len--;
	}
	return 0;
}

/* Classic multiplication
 * - intr = int1 * int2
 * - int1, int2 are not zero
 * - intr must have enough space to hold result
 * - intr is zeroed
 * - return result length
 */
static int mul_buf_classic(uint *intr, const uint *int1, const int msb1,
		const uint *int2, const int msb2)
{
	int msb = 1;

	for (int i = 0; i < msb1 + msb2; i++) {
		int jmax = i;
		if (jmax >= msb1)
			jmax = msb1 - 1;
		int jmin = i - msb2 + 1;
		if (jmin < 0)
			jmin = 0;

#ifdef __SIZEOF_INT128__
		__uint128_t tmp128 = 0;
		uint tmp[5];
		for (int j = jmax, k = i - jmax; j >= jmin; j--, k++)
			tmp128 += (uint64_t)int1[j] * int2[k];
		int msbt = from128(tmp, tmp128);
		msb = add_buf(intr+i, msb-i, tmp, msbt) + i;
#else
		uint tmp[3];
		for (int j = jmax, k = i - jmax; j >= jmin; j--, k++) {
			uint64_t tmp64 = (uint64_t)int1[j] * int2[k];
			int msbt = from64(tmp, tmp64);
			msb = add_buf(intr+i, msb-i, tmp, msbt) + i;
		}
#endif
	}

	/* Top int may be 0 */
	if (intr[msb-1] == 0 && msb > 1)
		msb--;

	tt_assert_fa(msb == (msb1+msb2) || msb == (msb1+msb2-1));
	return msb;
}

/* Karatsuba multiplication
 *        A   B  <- int1
 *    x)  C   D  <- int2
 *  -----------
 *  AC AD+BC BD  <- intr
 *
 * - intr = int1 * int2
 * - int1, int2 are not zero
 * - intr must have enough space to hold result
 * - intr is zeroed
 * - return result length
 */
static int mul_buf_kara(uint *intr, const uint *int1, const int msb1,
		const uint *int2, const int msb2, uint *workbuf)
{
	/* Fallback to classic algorithm */
	if (msb1 < KARA_CROSS && msb2 < KARA_CROSS)
		return mul_buf_classic(intr, int1, msb1, int2, msb2);

	int div = _tt_max(msb1, msb2) / 2;
	int msb_a = 0, msb_b = msb1;
	int msb_c = 0, msb_d = msb2;
	if (div < msb1) {
		msb_a = msb1 - div;
		msb_b = get_uints(int1, div);
	}
	if (div < msb2) {
		msb_c = msb2 - div;
		msb_d = get_uints(int2, div);
	}

	/* Result = (A*C)*base^(div*2) + (A*D+B*C)*base^div + B*D */
	int msb = 1;
	int msb_ac = 0, msb_bd = 0;

	/* B*D -> intr */
	if (msb_b && msb_d) {
		msb_bd = mul_buf_kara(intr, int1, msb_b, int2, msb_d, workbuf);
		msb = msb_bd;
	}

	/* A*C -> intr+div*2 */
	if (msb_a && msb_c) {
		msb_ac = mul_buf_kara(intr+div*2, int1+div, msb_a,
				int2+div, msb_c, workbuf);
		msb = msb_ac + div*2;
	}

	/* A*D+B*C */
	if (!((msb_a == 0 || msb_d == 0) && (msb_b == 0 || msb_c == 0))) {
		uint *a_b = workbuf;
		uint *c_d = a_b + (div + 2);
		uint *ad_bc = c_d + (div + 2);
		uint *nextbuf = ad_bc + 2 * (div + 4);
		const int bufsz = div * 4 + 12;
		memset(workbuf, 0, bufsz * 4);

		/* A+B */
		int msb_a_b = 1;
		if (msb_a) {
			memcpy(a_b, int1+div, msb_a*4);
			msb_a_b = msb_a;
		}
		if (msb_b)
			msb_a_b = add_buf(a_b, msb_a_b, int1, msb_b);

		/* C+D */
		int msb_c_d = 1;
		if (msb_c) {
			memcpy(c_d, int2+div, msb_c*4);
			msb_c_d = msb_c;
		}
		if (msb_d)
			msb_c_d = add_buf(c_d, msb_c_d, int2, msb_d);

		/* A*D+B*C = (A+B)*(C+D)-A*C-B*D */
		int msb_ad_bc = mul_buf_kara(ad_bc, a_b, msb_a_b, c_d, msb_c_d,
				nextbuf);
		if (msb_ac)
			msb_ad_bc = sub_buf(ad_bc, msb_ad_bc, intr+div*2, msb_ac);
		if (msb_bd)
			msb_ad_bc = sub_buf(ad_bc, msb_ad_bc, intr, msb_bd);

		/* Sum all */
		int msb2 = msb - div;
		if (msb2 <= 0)
			msb2 = 1;
		msb = add_buf(intr+div, msb2, ad_bc, msb_ad_bc) + div;
	}

	tt_assert_fa(msb == (msb1+msb2) || msb == (msb1+msb2-1));
	return msb;
}

/* Guess quotient by high digits */
static uint guess_quotient(const uint *_src1, int msb1,
		const uint *_src2, int msb2)
{
	if (msb1 < msb2)
		return 0;

	_src1 += (msb1 - 1);
	_src2 += (msb2 - 1);

#ifdef __SIZEOF_INT128__
	__uint128_t dividend;
	uint64_t divisor;
#else
	double dividend, divisor;
#endif

	dividend = *_src1--;
	if (--msb1) {
		dividend *= BIT(31);
		dividend += *_src1--;
		if (--msb1) {
			dividend *= BIT(31);
			dividend += *_src1;
		}
	}
	divisor = *_src2--;;
	if (--msb2) {
		divisor *= BIT(31);
		divisor += *_src2;
	}

	uint q = (uint)(dividend / divisor);
	if (q >= BIT(31))
		q = BIT(31) - 1;
	return q;
}

/* Divide: _qt = _dd / _ds; _rm = _dd % _ds
 * - _qt, _rm, _msb_qt, _msb_rm may be NULL
 * - _qt: input - zeroed; output - quotient
 * - *_msb_qt: input - size of _qt[]; output - quotient msb
 * - _rm: input - zeroed; output - remainder
 * - *_msb_rm: input - size of _rm[]; output - remainder msb
 * - _dd, msb_dd: dividend
 * - _ds, msb_ds: divisor
 * - msb_dd >= msb_ds, _dd >= _ds
 */
static int div_buf_classic(uint *_qt, int *_msb_qt, uint *_rm, int *_msb_rm,
		const uint *_dd, int msb_dd, const uint *_ds, int msb_ds)
{
	tt_assert(_qt == NULL || *_msb_qt > (msb_dd-msb_ds));
	tt_assert(_rm == NULL || *_msb_rm >= msb_ds);
	tt_assert((_qt || _rm) && msb_dd >= msb_ds);

	/* Reuse remainder buffer to store normalized divisor */
	uint *qt = _qt, *ds = _rm;
	int *msb_qt = _msb_qt, __msb_qt;
	if (msb_qt == NULL)
		msb_qt = &__msb_qt;	/* Just to simplify code */

	/* Shift bits to normalize divisor */
	const int shift = __builtin_clz(_ds[msb_ds-1]) - 1;
	if (shift == 0)
		ds = (uint *)_ds;

	/* Allocate working buffer */
	const int sz_dd = msb_dd + 1;
	const int sz_tmpmul = sz_dd + 1;
	int sz_qt = 0, sz_ds = 0;
	if (qt == NULL)
		sz_qt = msb_dd - msb_ds + 1;
	if (ds == NULL)
		sz_ds = msb_ds;

	uint *workbuf = calloc(sz_dd + sz_tmpmul + sz_qt + sz_ds, 4);
	if (workbuf == NULL)
		return TT_ENOMEM;
	uint *dd = workbuf;
	uint *tmpmul = dd + sz_dd;
	if (qt == NULL)
		qt = tmpmul + sz_tmpmul;
	if (ds == NULL)
		ds = tmpmul + sz_tmpmul + sz_qt;

	/* Normalize: msb(divisor) = 1 */
	if (shift == 0) {
		memcpy(dd, _dd, msb_dd * 4);
	} else {
		int i;
		uint tmp = 0;

		for (i = 0; i < msb_ds; i++) {
			ds[i] = ((_ds[i] << shift) | tmp) & ~BIT(31);
			tmp = _ds[i] >> (31 - shift);
		}
		tt_assert(tmp == 0);

		for (i = 0; i < msb_dd; i++) {
			dd[i] = ((_dd[i] << shift) | tmp) & ~BIT(31);
			tmp = _dd[i] >> (31 - shift);
		}
		if (tmp) {
			dd[i] = tmp;
			msb_dd++;
		}
	}

	int qt_idx, topmsb = msb_ds;
	uint *dd_top = dd + msb_dd - msb_ds;

	*msb_qt = msb_dd - msb_ds;
	qt_idx = *msb_qt - 1;
	if (cmp_buf(dd_top, msb_ds, ds, msb_ds) >= 0) {
		/* First digit */
		qt[(*msb_qt)++] = 1;
		topmsb = sub_buf(dd_top, msb_ds, ds, msb_ds);
	}
	if (*msb_qt == 0)
		*msb_qt = 1;

	while (qt_idx >= 0) {
		if (!(topmsb == 1 && *dd_top == 0))
			topmsb++;
		dd_top--;

		uint q = guess_quotient(dd_top, topmsb, ds, msb_ds);
		if (q) {
			memset(tmpmul, 0, sz_tmpmul * 4);
			int mulmsb = mul_buf_classic(tmpmul, ds, msb_ds, &q, 1);
			while (cmp_buf(tmpmul, mulmsb, dd_top, topmsb) > 0) {
				mulmsb = sub_buf(tmpmul, mulmsb, ds, msb_ds);
				q--;
			}
			tt_assert_fa(topmsb >= mulmsb);
			topmsb = sub_buf(dd_top, topmsb, tmpmul, mulmsb);
		}
		qt[qt_idx--] = q;
	}
	tt_assert_fa(dd_top == dd);

	/* dd[topmsb]>>shift -> remainder */
	if (_rm) {
		if (shift) {
			uint tmp = 0, tmp2;
			for (int i = topmsb-1; i >= 0; i--) {
				tmp2 = dd[i];
				dd[i] = (dd[i] >> shift) | tmp;
				tmp = (tmp2 << (31 - shift)) & ~BIT(31);
			}
			if (dd[topmsb-1] == 0 && topmsb > 1)
				topmsb--;
		}
		tt_assert_fa(topmsb <= msb_ds);
		memcpy(_rm, dd, topmsb * 4);
		*_msb_rm = topmsb;
	}

	free(workbuf);
	return 0;
}

/* intr = int1 * int2
 * - int1, msb, int2, msb2 must from valid tt_int
 * - intr is zeroed on enter
 * - return result length, -1 if no enough memory
 */
int _tt_int_mul_buf(uint *intr, const uint *int1, const int msb1,
		const uint *int2, const int msb2)
{
	/* mul_buf_xxx cannot treat 0 correctly */
	if ((msb1 == 1 && int1[0] == 0) || (msb2 == 1 && int2[0] == 0))
		return 1;

	/* Use classic algorithm when input below crosspoint */
	if (msb1 < KARA_CROSS && msb2 < KARA_CROSS)
		return mul_buf_classic(intr, int1, msb1, int2, msb2);

	/* Allocate working buffer for Karatsuba algorithm */
	const int worksz = (_tt_max(msb1, msb2) + 6) * 4;
	uint *workbuf = malloc(worksz*4);
	if (workbuf == NULL)
		return TT_ENOMEM;

	int ret = mul_buf_kara(intr, int1, msb1, int2, msb2, workbuf);

	free(workbuf);
	return ret;
}

/* dst = src1 + src2. dst may share src1 or src2. */
int tt_int_add(struct tt_int *dst, const struct tt_int *src1,
		const struct tt_int *src2)
{
	return add_sub_ints(dst, src1, src2, 0);
}

/* dst = src1 - src2. dst may share src1 or src2. */
int tt_int_sub(struct tt_int *dst, const struct tt_int *src1,
		const struct tt_int *src2)
{
	return add_sub_ints(dst, src1, src2, 1);
}

/* dst = src1 * src2. dst may share src1 or src2. */
int tt_int_mul(struct tt_int *dst, const struct tt_int *src1,
		const struct tt_int *src2)
{
	if (_tt_int_is_zero(src1) || _tt_int_is_zero(src2)) {
		_tt_int_zero(dst);
		return 0;
	}

	/* Allocate result buffer */
	uint *r = calloc(src1->_msb + src2->_msb, 4);
	if (!r)
		return TT_ENOMEM;

	const int sign = src1->_sign ^ src2->_sign;
	uint msb = _tt_int_mul_buf(r, src1->_int, src1->_msb,
			src2->_int, src2->_msb);
	if (dst->_max < msb) {
		int ret = _tt_int_realloc(dst, msb);
		if (ret)
			return ret;
	} else {
		_tt_int_zero(dst);
	}
	memcpy(dst->_int, r, msb * 4);
	dst->_msb = msb;
	dst->_sign = sign;

	free(r);
	return 0;
}

/* quo = src1 / src2, rem = src1 % src2.
 * - dst, src may share src1 or src2
 * - quo or rem may be null
 */
int tt_int_div(struct tt_int *quo, struct tt_int *rem,
		const struct tt_int *src1, const struct tt_int *src2)
{
	int ret = 0;
	tt_assert(quo != rem);

	/* Check zero */
	if (_tt_int_is_zero(src2))
		return TT_APN_EDIV_0;

	/* dividend < divisor */
	if (tt_int_cmp_abs(src1, src2) < 0) {
		if (rem && rem != src1)
			ret = _tt_int_copy(rem, src1);
		_tt_barrier();
		if (quo)
			_tt_int_zero(quo);
		return ret;
	}

	/* Get sign of quotient and remainder */
	const int sign_quo = src1->_sign ^ src2->_sign;
	const int sign_rem = src1->_sign;

	/* Allocate working buffer: quotient, remainder */
	uint *qt = NULL, *rm = NULL;
	int msb_qt = src1->_msb - src2->_msb + 1, msb_rm = src2->_msb;
	if (quo == NULL)
		msb_qt = 0;
	if (rem == NULL)
		msb_rm = 0;
	uint *workbuf = calloc(msb_qt + msb_rm, 4);
	if (workbuf == NULL)
		return TT_ENOMEM;
	if (quo)
		qt = workbuf;
	if (rem)
		rm = workbuf + msb_qt;

	/* Do division */
	ret = div_buf_classic(qt, &msb_qt, rm, &msb_rm,
		src1->_int, src1->_msb, src2->_int, src2->_msb);
	if (ret)
		goto out;

	/* qt[msb_qt] -> quotient */
	if (quo) {
		_tt_int_zero(quo);
		ret = _tt_int_realloc(quo, msb_qt);
		if (ret)
			goto out;
		quo->_sign = sign_quo;
		quo->_msb = msb_qt;
		memcpy(quo->_int, qt, msb_qt * 4);
	}

	/* rm[msb_rm] -> remainder */
	if (rem) {
		_tt_int_zero(rem);
		ret = _tt_int_realloc(rem, msb_rm);
		if (ret)
			goto out;
		rem->_sign = sign_rem;
		rem->_msb = msb_rm;
		memcpy(rem->_int, rm, msb_rm * 4);
	}

out:
	free(workbuf);
	return ret;
}

/* Compare absolute value
 * - return: 1 - src1 > src2, 0 - src1 == src2, -1 - src1 < src2
 */
int tt_int_cmp_abs(const struct tt_int *src1, const struct tt_int *src2)
{
	return cmp_buf(src1->_int, src1->_msb, src2->_int, src2->_msb);
}

int tt_int_cmp(const struct tt_int *src1, const struct tt_int *src2)
{
	int ret;

	if (src1->_sign != src2->_sign) {
		if (_tt_int_is_zero(src1) && _tt_int_is_zero(src2))
			return 0;
		ret = src2->_sign - src1->_sign;
	} else {
		ret = tt_int_cmp_abs(src1, src2);
		if (src1->_sign)
			ret = -ret;
	}

	return ret;
}
