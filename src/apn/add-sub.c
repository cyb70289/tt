/* Add, Sub
 *
 * Copyright (C) 2015 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/apn/apn.h>
#include <common/lib.h>
#include "apn.h"

#include <string.h>

#pragma GCC diagnostic ignored "-Wunused-variable"

/* Add two uint(9 digit decimals) and re-arrange
 * - carry: input: carry bit of top 3 digits of lower uint
 * - carry: output: carry bit of top 3 digits of this uint
 * return arranged digit
 */
static uint add_dig(uint dig, uint dig2, char *carry)
{
	dig = _tt_add_uint(dig + *carry, dig2, carry);

	/* Split to 3-digit sets */
	uint xdig[3];	/* Extended digit with 11 bits */
	xdig[0] = dig & 0x7FF;
	xdig[1] = (dig >> 11) & 0x7FF;
	xdig[2] = dig >> 22;
	if (*carry)
		xdig[2] |= BIT(10);

	/* Check overflow */
	if (xdig[0] > 999) {
		xdig[0] -= 1000;
		xdig[1]++;
	}
	if (xdig[1] > 999) {
		xdig[1] -= 1000;
		xdig[2]++;
	}
	if (xdig[2] > 999) {
		xdig[2] -= 1000;
		*carry = 1;
	}

	/* Combine to one uint */
	return (xdig[2] << 22) | (xdig[1] << 11) | xdig[0];
}

/* dig += dig2
 * - msb: digit length, may <= 0
 * - dig must have enouth space to hold result(include possible carry)
 * - return result digit length
 */
static int add_digs(uint *dig, const int msb, const uint *dig2, const int msb2)
{
	int i;
	char carry = 0;

	/* Add dig2 to dig */
	const int uints2 = (msb2 + 8) / 9;
	for (i = 0; i < uints2; i++)
		dig[i] = add_dig(dig[i], dig2[i], &carry);

	/* Process remaining high digits in dig */
	const int uints = (msb + 8) / 9;
	for (; i < uints; i++) {
		if (carry == 0) {
			i = uints;
			break;
		}
		dig[i] = add_dig(dig[i], 0, &carry);
	}

	/* Check new MSB */
	if (carry) {
		/* Carry at uint boundary */
		dig[i] = 1;
		return _tt_max(msb, msb2) + 1;
	} else {
		/* i => valid uints */
		tt_assert_fa(i > 0);
		uint u = dig[i-1];
		/* Get new msb */
		int msbr = (i - 1) * 9;
		if ((u >> 22))
			msbr += (_tt_digits(u >> 22) + 6);
		else if ((u >> 11) & 0x3FF)
			msbr += (_tt_digits((u >> 11) & 0x3FF) + 3);
		else
			msbr += _tt_digits(u & 0x3FF);
		const int msbs = _tt_max(msb, msb2);
		tt_assert_fa(msbr == msbs || msbr == (msbs+1));
		return msbr;
	}
}

/* Sub two uint(9 digit decimals) and re-arrange
 * - carry: input: carry bit of top 3 digits of lower uint
 * - carry: output: carry bit of top 3 digits of this uint
 * return arranged digit
 */
static uint sub_dig(uint dig, uint dig2, char *carry)
{
	dig |= (BIT(10) | BIT(21));
	dig = _tt_sub_uint(dig - *carry, dig2, carry);

	/* Split to 3-digit sets */
	uint xdig[3];	/* Extended digit with 11 bits */
	xdig[0] = dig & 0x7FF;
	xdig[1] = (dig >> 11) & 0x7FF;
	xdig[2] = dig >> 22;
	if (*carry) {
		xdig[2] = ~xdig[2];
		xdig[2] &= 0x3FF;
		xdig[2] = 1023 - xdig[2];
	} else {
		xdig[2] += 1024;
	}

	/* Check underflow */
	if (xdig[0] < 1024) {
		xdig[0] -= 24;
		xdig[1]--;
	} else {
		xdig[0] -= 1024;
	}
	if (xdig[1] < 1024) {
		xdig[1] -= 24;
		xdig[2]--;
	} else {
		xdig[1] -= 1024;
	}
	if (xdig[2] < 1024) {
		xdig[2] -= 24;
		*carry = 1;
	} else {
		xdig[2] -= 1024;
	}

	/* Combine to one uint */
	return (xdig[2] << 22) | (xdig[1] << 11) | xdig[0];
}

/* dig = dig1 - dig2
 * - dig1 >= dig2
 * - msb: digit length, may <= 0
 * - dig must have enouth space to hold result
 * - dig may share buffer with dig1 or dig2
 * - dig must be zeroed if not shared with dig1 or dig2
 * - return result digit length
 */
static int sub_digs(uint *dig, const uint *dig1, const int msb1,
		const uint *dig2, const int msb2)
{
	tt_assert_fa(msb1 >= msb2);

	int i, msbr = 1;
	char carry = 0;

	/* dig1 - dig2 */
	const int uints2 = (msb2 + 8) / 9;
	for (i = 0; i < uints2; i++)
		dig[i] = sub_dig(dig1[i], dig2[i], &carry);

	/* Process remaining high digits in dig1 */
	const int uints1 = (msb1 + 8) / 9;
	for (; i < uints1; i++)
		dig[i] = sub_dig(dig1[i], 0, &carry);
	tt_assert_fa(carry == 0);

	/* Check new MSB */
	for (i = uints1 - 1; i >= 0; i--) {
		uint u = dig[i];
		if (u) {
			/* Get msb */
			msbr = i * 9;
			if ((u >> 22))
				msbr += (_tt_digits(u >> 22) + 6);
			else if ((u >> 11) & 0x3FF)
				msbr += (_tt_digits((u >> 11) & 0x3FF) + 3);
			else
				msbr += _tt_digits(u & 0x3FF);
			break;
		}
	}
	tt_assert_fa((msbr <= msb1 || msb1 <= 0) && msbr >= 1);

	return msbr;
}

/* Compare digits
 * - msb: digit length, may <= 0
 * - return: 1 - dig1 > dig2, 0 - dig1 == dig2, -1 - dig1 < dig2
 */
static int cmp_digs(const uint *dig1, const int msb1,
		const uint *dig2, const int msb2)
{
	if (msb1 <= 0 && msb2 <= 0)
		return 0;
	if (msb1 < msb2)
		return -1;
	if (msb1 > msb2)
		return 1;

	/* Compare digits: msb1 = msb2 > 0 */
	const int uints = (msb1 + 8) / 9;
	for (int i = uints - 1; i >= 0; i--) {
		if (dig1[i] > dig2[i])
			return 1;
		else if (dig1[i] < dig2[i])
			return -1;
	}
	return 0;
}

/* Left shift adj digits
 * - adj = 1, 2
 * - return shifted value
 */
static uint lshift_dig12(uint dig32, int adj)
{
	uchar d[9];
	_tt_apn_to_d3_cp(dig32 & 0x3FF, d);
	_tt_apn_to_d3_cp((dig32 >> 11) & 0x3FF, d+3);
	_tt_apn_to_d3_cp(dig32 >> 22, d+6);

	int i;
	for (i = 8; i >= adj; i--)
		d[i] = d[i-adj];
	while (i >= 0)
		d[i--] = 0;

	return (_tt_apn_from_d3(d+6) << 22) | (_tt_apn_from_d3(d+3) << 11) |
		_tt_apn_from_d3(d);
}

/* Right shift adj digits
 * - adj = 1, 2
 * - retrun shifted value
 */
static uint rshift_dig12(uint dig32, int adj)
{
	uchar d[9];
	_tt_apn_to_d3_cp(dig32 & 0x3FF, d);
	_tt_apn_to_d3_cp((dig32 >> 11) & 0x3FF, d+3);
	_tt_apn_to_d3_cp(dig32 >> 22, d+6);

	int i;
	for (i = 0; i < 9-adj; i++)
		d[i] = d[i+adj];
	while (i < 9)
		d[i++] = 0;

	return (_tt_apn_from_d3(d+6) << 22) | (_tt_apn_from_d3(d+3) << 11) |
		_tt_apn_from_d3(d);
}

/* Shift "src" adj digits and copy to "dst"
 * - msb: digits in src
 * - adj: - -> right/div, + -> left/mul
 * - (msb + adj) may <= 0
 * - dst and src may share same buffer
 * - dst should be zeroed if not share with src
 * - dst must have enough space to hold shifted src
 * - return result digit length
 */
static int shift_digs(uint *dst, const uint *src, int msb, int adj)
{
	int uints = (msb + 8) / 9;	/* Valid uints in src */
	int msb_r = msb + adj;		/* Result length for debugging */

	/* Check shift direction */
	int append0;
	if (adj < 0) {
		adj = -adj;
		if (adj >= msb) {
			/* All digits truncated, just fill with 0 */
			if (dst == src)
				memset(dst, 0, uints * 4);
			return 1;
		}
		append0 = 0;	/* Truncate */
	} else if (adj > 0) {
		append0 = 1;	/* Append */
	} else {
		if (dst != src)
			memcpy(dst, src, uints * 4);
		return msb;		/* No shift */
	}

	/* Shift */
	int adj9 = adj / 9;	/* Align to uint */
	adj %= 9;
	int adj3 = adj / 3;	/* Align to 10bit */
	adj %= 3;
	int i;
	const uint *cur = src;	/* Current digit buffer */
	if (append0) { /* Left shift */
		/* Align-9 */
		if (adj9) {
			for (i = uints-1; i >= 0; i--)
				dst[i+adj9] = src[i];
			memset(dst, 0, adj9 * 4);

			msb += adj9 * 9;
			uints += adj9;

			cur = dst;
		}

		/* Align-3 */
		if (adj3) {	/* adj3 = 1, 2 */
			/* Check 3-digit sets in first uint */
			int u1_dig3 = (msb - 1) % 9 + 1;
			u1_dig3 += 2;
			u1_dig3 /= 3;	/* 1, 2, 3 */
			if ((u1_dig3 + adj3) > 3)
				uints++;
			msb += adj3 * 3;

			/* Shift left */
			static const int sfl_bits[] = { 11, 22 };
			static const int sfr_bits[] = { 22, 11 };
			for (i = uints-1; i > 0; i--) {
				dst[i] = cur[i] << sfl_bits[adj3-1];
				dst[i] |= cur[i-1] >> sfr_bits[adj3-1];
			}
			dst[0] = cur[0] << sfl_bits[adj3-1];

			cur = dst;
		}

		/* Unaligned */
		if (adj) {	/* adj = 1, 2 */
			/* Check digits in first uint */
			int u1_dig = (msb - 1) % 9 + 1;
			if ((u1_dig + adj) > 9)
				uints++;
			msb += adj;

			/* Shift left */
			for (i = uints-1; i > 0; i--) {
				dst[i] = lshift_dig12(cur[i], adj);
				/* Append msb of lower uint */
				const uchar *d = _tt_apn_to_d3(cur[i-1] >> 22);
				if (adj == 1)
					dst[i] += d[2];
				else
					dst[i] += (d[2] * 10 + d[1]);
			}
			dst[0] = lshift_dig12(cur[0], adj);
		}
	} else { /* Right shift */
		/* Align-9 */
		if (adj9) {
			for (i = adj9; i < uints; i++)
				dst[i-adj9] = src[i];
			memset(dst + uints - adj9, 0, adj9 * 4);

			msb -= adj9 * 9;
			uints -= adj9;

			cur = dst;
		}

		/* Align-3 */
		if (adj3) {	/* adj3 = 1, 2 */
			/* Shift right */
			static const int sfl_bits[] = { 22, 11 };
			static const int sfr_bits[] = { 11, 22 };
			for (i = 0; i < uints-1; i++) {
				dst[i] = cur[i] >> sfr_bits[adj3-1];
				dst[i] |= cur[i+1] << sfl_bits[adj3-1];
			}
			dst[uints-1] = cur[uints-1] >> sfr_bits[adj3-1];

			cur = dst;

			/* Check 3-digit sets in first uint */
			int u1_dig3 = (msb - 1) % 9 + 1;
			u1_dig3 += 2;
			u1_dig3 /= 3;	/* 1, 2, 3 */
			if (u1_dig3 <= adj3)
				uints--;
			msb -= adj3 * 3;
		}

		/* Unaligned */
		if (adj) {	/* adj = 1, 2 */
			/* Shift right */
			for (i = 0; i < uints - 1; i++) {
				dst[i] = rshift_dig12(cur[i], adj);
				/* Prepend lsb of higher uint */
				const uchar *d = _tt_apn_to_d3(cur[i+1] & 0x3FF);
				uint a;
				if (adj == 1)
					a = ((uint)d[0]) * 100;
				else
					a = ((uint)d[1]) * 100 + d[0] * 10;
				dst[i] += a << 22;
			}
			dst[uints-1] = rshift_dig12(cur[uints-1], adj);

			/* Check digits in first uint */
			int u1_dig = (msb - 1) % 9 + 1;
			if (u1_dig <= adj)
				uints--;
			msb -= adj;
		}
	}

	tt_assert_fa(msb_r == msb);
	return msb;
}

/* 10^n, n = 0 ~ 8 */
static const uint one_tbl[] = {
	1, 10, 100,
	1U << 11, 10U << 11, 100U << 11,
	1U << 22, 10U << 22, 100U << 22,
};

/* dst = src1 +/- src2
 * - sub: 0 -> add, 1 -> sub
 * - dst may share with src1 or src2
 */
static int add_sub_apn(struct tt_apn *dst, const struct tt_apn *src1,
		const struct tt_apn *src2, int sub)
{
	int ret = 0;

	int sign1 = src1->_sign;
	int sign2 = src2->_sign;
	if (sub)
		sign2 = !sign2;

	/* Check NaN, Inf */
	if (src1->_inf_nan == TT_APN_NAN || src2->_inf_nan == TT_APN_NAN) {
		dst->_inf_nan = TT_APN_NAN;
		return TT_APN_EINVAL;
	}
	if (src1->_inf_nan == TT_APN_INF || src2->_inf_nan == TT_APN_INF) {
		dst->_inf_nan = TT_APN_INF;
		dst->_sign = (src1->_inf_nan == TT_APN_INF ? sign1 : sign2);
		return TT_APN_EOVERFLOW;
	}

	/* Allocate a new APN if dst and src overlaps */
	struct tt_apn *dst2 = dst;
	if (dst == src1 || dst == src2) {
		dst2 = tt_apn_alloc(dst->_prec);
		if (!dst2)
			return TT_ENOMEM;
	} else {
		_tt_apn_zero(dst);
	}

	/* Check exponent alignment.
	 * Swap src1 and src2 if exponent of src1 is smaller, and make src1
	 * the one needs to adjust exponent(left shift signifcands).
	 */
	int exp_adj1 = src1->_exp - src2->_exp, exp_adj2 = 0;
	if (exp_adj1 < 0) {
		exp_adj1 = -exp_adj1;
		__tt_swap(src1, src2);
		__tt_swap(sign1, sign2);
	}

	/* Check rounding */
	int msb_dst = src1->_msb;
	/* Shifting "0" can't influence precision */
	if (!_tt_apn_is_zero(src1))
		msb_dst += exp_adj1;
	if (msb_dst < src2->_msb)
		msb_dst = src2->_msb;
	if (msb_dst > dst2->_prec) {
		int adj_adj = msb_dst - dst2->_prec;
		exp_adj1 -= adj_adj;
		exp_adj2 -= adj_adj;
	}
	if ((exp_adj1 < 0 && !_tt_apn_is_true_zero(src1)) ||
			(exp_adj2 < 0 && !_tt_apn_is_true_zero(src2)))
		ret = TT_APN_EROUNDED;

	/* Set result exponent, may adjust later */
	dst2->_exp = src1->_exp - exp_adj1;

	/* Use rounding guard digits to gain precision */
	int adj_adj = 0;
	if (exp_adj2 < 0) {
		int guard_adj = -exp_adj2;
		if (guard_adj > TT_APN_PREC_RND)
			guard_adj = TT_APN_PREC_RND;
		if (-(exp_adj2 + guard_adj) < src2->_msb) {
			exp_adj1 += guard_adj;
			exp_adj2 += guard_adj;
			adj_adj += guard_adj;
		}
	}

	/* If both operands need shift, make src1 align-9 shifting */
	if ((exp_adj1 % 9) && (exp_adj2 % 9)) {
		int align_adj = 9 - exp_adj1 % 9;
		if (align_adj > 9)	/* exp_adj may be negative */
			align_adj -= 9;
		exp_adj1 += align_adj;
		exp_adj2 += align_adj;
		adj_adj += align_adj;
	}

	/* "0" needn't shift */
	if (_tt_apn_is_zero(src1))
		exp_adj1 = 0;
	if (_tt_apn_is_zero(src2))
		exp_adj2 = 0;

	/* Copy adjusted significand of src1 to dst */
	tt_assert_fa(dst2->_prec_full > (src1->_msb + exp_adj1));
	shift_digs(dst2->_dig32, src1->_dig32, src1->_msb, exp_adj1);

	/* Copy adjusted significand of src2 to temporary buffer */
	uint *tmpdig = malloc(dst2->_digsz);
	memset(tmpdig, 0, dst2->_digsz);
	tt_assert_fa(dst2->_prec_full > (src2->_msb + exp_adj2));
	shift_digs(tmpdig, src2->_dig32, src2->_msb, exp_adj2);

	/* Compare sign, decide to do "+" or "-" */
	int msb;
	if (sign1 == sign2) {
		/* Adding... */
		dst2->_sign = sign1;
		/* Add aligned significands */
		msb = add_digs(dst2->_dig32, src1->_msb + exp_adj1,
				tmpdig, src2->_msb + exp_adj2);
	} else {
		/* Substracting... */
		/* Pick bigger value */
		int cmp12 = cmp_digs(dst2->_dig32, src1->_msb + exp_adj1,
				tmpdig, src2->_msb + exp_adj2);
		/* Substract aligned significands */
		if (cmp12 >= 0) {
			dst2->_sign = sign1;
			msb = sub_digs(dst2->_dig32,
					dst2->_dig32, src1->_msb + exp_adj1,
					tmpdig, src2->_msb + exp_adj2);
		} else {
			dst2->_sign = sign2;
			msb = sub_digs(dst2->_dig32,
					tmpdig, src2->_msb + exp_adj2,
					dst2->_dig32, src1->_msb + exp_adj1);
		}
	}

	/* Check rounding */
	if (adj_adj) {
		if (_tt_round(_tt_apn_get_dig(dst2->_dig32, adj_adj) & 1,
				_tt_apn_get_dig(dst2->_dig32, adj_adj-1), 0)) {
			/* Construct 10^adj_adj */
			tt_assert_fa(adj_adj < 36);
			uint one[4] = { 0, 0, 0, 0 };
			one[adj_adj / 9] = one_tbl[adj_adj % 9];
			msb = add_digs(dst2->_dig32, msb, one, adj_adj+1);
		}
	}
	/* Adjust significand, msb */
	dst2->_msb = msb - adj_adj;
	/* Only happen on substracting */
	if (dst2->_msb <= 0)
		dst2->_msb = 1;
	tt_assert_fa(dst2->_msb >= 1 && dst2->_msb <= (dst2->_prec+1));
	/* Only happen on adding */
	if (dst2->_msb > dst2->_prec) {
		adj_adj++;
		dst2->_msb--;
		dst2->_exp++;	/* TODO: xflow */
		ret = TT_APN_EROUNDED;
	}
	shift_digs(dst2->_dig32, dst2->_dig32, msb, -adj_adj);

	free(tmpdig);

	/* Switch buffer if dst overlap with src */
	if (dst2 != dst) {
		free(dst->_dig32);
		memcpy(dst, dst2, sizeof(struct tt_apn));
		free(dst2);
	}

	if (ret == TT_APN_EROUNDED)
		tt_debug("APN rounded");

	return ret;
}

/* dst = src1 + src2. dst may share src1 or src2. */
int tt_apn_add(struct tt_apn *dst, const struct tt_apn *src1,
		const struct tt_apn *src2)
{
	return add_sub_apn(dst, src1, src2, 0);
}

/* dst = src1 - src2. dst may share src1 or src2. */
int tt_apn_sub(struct tt_apn *dst, const struct tt_apn *src1,
		const struct tt_apn *src2)
{
	return add_sub_apn(dst, src1, src2, 1);
}
