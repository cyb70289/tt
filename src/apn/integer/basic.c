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
	if (workbuf == NULL) {
		tt_error("Out of memory");
		return TT_ENOMEM;
	}

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
	if (!r) {
		tt_error("Out of memory");
		return TT_ENOMEM;
	}

	int sign = src1->_sign ^ src2->_sign;
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

/* Compare absolute value
 * - return: 1 - src1 > src2, 0 - src1 == src2, -1 - src1 < src2
 */
int tt_int_cmp_abs(const struct tt_int *src1, const struct tt_int *src2)
{
	if (src1->_msb > src2->_msb)
		return 1;
	else if (src1->_msb < src2->_msb)
		return -1;

	for (int i = src1->_msb-1; i >= 0; i--) {
		if (src1->_int[i] > src2->_int[i])
			return 1;
		else if (src1->_int[i] < src2->_int[i])
			return -1;
	}
	return 0;
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
