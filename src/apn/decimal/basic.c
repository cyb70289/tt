/* Add, Sub, Multiply, Divide, Compare
 *
 * Copyright (C) 2015 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/apn/decimal.h>
#include <common/lib.h>
#include "decimal.h"

#include <string.h>

#pragma GCC diagnostic ignored "-Wunused-variable"

/* 10^n, n = 0 ~ 8 */
static const uint one_tbl[] = {
	1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000,
};

/* Get MSB of dig[0] ~ dig[uints-1] */
static int get_msb(const uint *dig, const int uints)
{
	int msb = 1;

	for (int i = uints - 1; i >= 0; i--) {
		uint u = dig[i];
		if (u) {
			msb = i * 9;
			do {
				msb++;
				u /= 10;
			} while (u);

			break;
		}
	}

	return msb;
}

/* Add two uint(9 digit decimals)
 * - carry: input: 0, 1
 * - carry: output: 1 => sum > 999,999,999
 * return adjust digit
 */
static uint add_dig(uint dig, uint dig2, char *carry)
{
	dig += (dig2 + *carry);

	/* Check overflow */
	if (dig >= 1000000000) {
		dig -= 1000000000;
		*carry = 1;
	} else {
		*carry = 0;
	}

	return dig;
}

/* dig += dig2
 * - msb: digit length, may <= 0
 * - dig must have enough space to hold result(include possible carry)
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
		/* Get new msb, i => valid uints */
		tt_assert_fa(i > 0);
		uint u = dig[i-1];
		int msbr = (i - 1) * 9;
		do {
			msbr++;
			u /= 10;
		} while (u);

		const int msbs = _tt_max(msb, msb2);
		tt_assert_fa(msbr == msbs || msbr == (msbs+1));
		return msbr;
	}
}

/* Sub two uint(9 digit decimals)
 * - carry: input: 0, 1
 * - carry: output: 1 => sub < 0
 * return adjust digit
 */
static uint sub_dig(uint dig, uint dig2, char *carry)
{
	int sub = dig - dig2 - *carry;
	if (sub < 0) {
		sub += 1000000000;
		*carry = 1;
	} else {
		*carry = 0;
	}

	return sub;
}

/* dig = dig1 - dig2
 * - dig1 >= dig2
 * - msb: digit length, may <= 0
 * - dig must have enough space to hold result
 * - dig may share buffer with dig1 or dig2
 * - dig must be zeroed if not shared with dig1 or dig2
 * - return result digit length
 */
static int sub_digs(uint *dig, const uint *dig1, const int msb1,
		const uint *dig2, const int msb2)
{
	tt_assert_fa(msb1 >= msb2);

	int i;
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
	int msbr = get_msb(dig, uints1);
	tt_assert_fa((msbr <= msb1 || msb1 <= 0) && msbr >= 1);

	return msbr;
}

/* *dig += ui * 10^(9*shift)
 * - ui in binary format
 * - *dig in decimal format
 * - shift: offset in uints, >= 0
 * - return MSB
 */
#ifdef __SIZEOF_INT128__
static inline int shift_add_u128(uint *dig, int msb, __uint128_t ui128,
		int shift)
{
	/* Convert *ui128 to decimal */
	uint dec[5] = { 0, 0, 0, 0, 0 };
	int msb2 = _tt_dec_uint128_to_dec(dec, ui128);

	/* Add dec[] to &dig[shift] */
	return add_digs(dig+shift, msb-shift*9, dec, msb2) + shift*9;
}
#else
static inline int shift_add_u64(uint *dig, int msb, uint64_t ui64, int shift)
{
	/* Convert ui64 to decimal */
	uint dec[3] = { 0, 0, 0 };
	int msb2 = _tt_dec_uint_to_dec(dec, ui64);

	/* Add dec[] to &dig[shift] */
	return add_digs(dig+shift, msb-shift*9, dec, msb2) + shift*9;
}
#endif

/* digr = dig1 * dig2
 * - msb: digit length, > 0
 * - dig1, dig2 are not zero
 * - digr must have enough space to hold result
 * - digr is zeroed
 * - return result digit length
 */
static int mul_digs(uint *digr, const uint *dig1, const int msb1,
		const uint *dig2, const int msb2)
{
	int msb = 1;
	const int uints1 = (msb1 + 8) / 9;
	const int uints2 = (msb2 + 8) / 9;

	for (int i = 0; i < uints1 + uints2; i++) {
		int jmax = i;
		if (jmax >= uints1)
			jmax = uints1 - 1;
		int jmin = i - uints2 + 1;
		if (jmin < 0)
			jmin = 0;

#ifdef __SIZEOF_INT128__
		/* XXX: int128 may overflow if both operands >= 10^(9*10^20) */
		__uint128_t tmp128 = 0;
		for (int j = jmax, k = i - jmax; j >= jmin; j--, k++)
			tmp128 += (uint64_t)dig1[j] * dig2[k];
		if (tmp128)
			msb = shift_add_u128(digr, msb, tmp128, i);
#else
		uint64_t tmp64 = 0;
		int cnt = 0;
		for (int j = jmax, k = i - jmax; j >= jmin; j--, k++) {
			tmp64 += (uint64_t)dig1[j] * dig2[k];
			cnt++;
			if (cnt == 10) {
				/* Consume before tmp64 overflow */
				msb = shift_add_u64(digr, msb, tmp64, i);
				tmp64 = 0;
				cnt = 0;
			}
		}
		if (cnt)
			msb = shift_add_u64(digr, msb, tmp64, i);
#endif
	}

	tt_assert_fa(msb >= msb1 && msb <= (msb1 + msb2));

	return msb;
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
 * - adj = 1~8
 * - return shifted value
 */
static inline uint lshift_dig_1_8(uint dig32, int adj)
{
	uint64_t dig64 = dig32;
	dig64 *= one_tbl[adj];
	return dig64 % 1000000000;
}

/* Right shift adj digits
 * - adj = 1~8
 * - retrun shifted value
 */
static inline uint rshift_dig_1_8(uint dig32, int adj)
{
	return dig32 / one_tbl[adj];
}

/* Shift "src" adj digits and copy to "dst"
 * - msb: digits in src
 * - dst_sz: buffer size of dst
 * - adj: - -> right/div, + -> left/mul
 * - (msb + adj) may <= 0
 * - dst and src may share same buffer
 * - dst should be zeroed if not share with src
 * - dst must have enough space to hold shifted src
 * - return result digit length
 */
static int shift_digs(uint *dst, uint dst_sz, const uint *src, int msb, int adj)
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
		return msb;	/* No shift */
	}

	/* Shift */
	int adj9 = adj / 9;	/* Align to uint */
	adj %= 9;
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

		/* Unaligned */
		if (adj) {	/* adj = 1~8 */
			/* Check digits in first uint */
			int u1_dig = (msb - 1) % 9 + 1;
			if ((u1_dig + adj) > 9)
				uints++;
			msb += adj;

			/* Shift left */
			for (i = uints-1; i > 0; i--) {
				dst[i] = lshift_dig_1_8(cur[i], adj);
				/* Append msb of lower uint */
				dst[i] += cur[i-1] / one_tbl[9-adj];
			}
			dst[0] = lshift_dig_1_8(cur[0], adj);
		}
	} else { /* Right shift */
		/* Align-9 */
		if (adj9) {
			for (i = adj9; i < uints; i++)
				dst[i-adj9] = src[i];
			int remains = dst_sz - (uints - adj9) * 4;
			if (remains > 0)
				memset(dst + uints - adj9, 0, remains);

			msb -= adj9 * 9;
			uints -= adj9;

			cur = dst;
		}

		/* Unaligned */
		if (adj) {	/* adj = 1~8 */
			/* Shift right */
			for (i = 0; i < uints - 1; i++) {
				dst[i] = rshift_dig_1_8(cur[i], adj);
				/* Prepend lsb of higher uint */
				dst[i] += (cur[i+1] % one_tbl[adj]) *
					one_tbl[9-adj];
			}
			dst[uints-1] = rshift_dig_1_8(cur[uints-1], adj);

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

/* dst = src1 +/- src2
 * - sub: 0 -> add, 1 -> sub
 * - dst may share with src1 or src2
 */
static int add_sub_dec(struct tt_dec *dst, const struct tt_dec *src1,
		const struct tt_dec *src2, int sub)
{
	int ret = 0;

	int sign1 = src1->_sign;
	int sign2 = src2->_sign;
	if (sub)
		sign2 = !sign2;

	/* Check NaN, Inf */
	if (src1->_inf_nan == TT_DEC_NAN || src2->_inf_nan == TT_DEC_NAN) {
		dst->_inf_nan = TT_DEC_NAN;
		return TT_APN_EINVAL;
	}
	if (src1->_inf_nan == TT_DEC_INF || src2->_inf_nan == TT_DEC_INF) {
		dst->_inf_nan = TT_DEC_INF;
		dst->_sign = (src1->_inf_nan == TT_DEC_INF ? sign1 : sign2);
		return TT_APN_EOVERFLOW;
	}
	dst->_inf_nan = 0;

	/* Allocate a new DEC if dst and src overlaps */
	struct tt_dec *dst2 = dst;
	if (dst == src1 || dst == src2) {
		dst2 = tt_dec_alloc(dst->_prec);
		if (!dst2)
			return TT_ENOMEM;
	} else {
		_tt_dec_zero(dst);
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
	if (!_tt_dec_is_zero(src1))
		msb_dst += exp_adj1;
	if (msb_dst < src2->_msb)
		msb_dst = src2->_msb;
	if (msb_dst > dst2->_prec) {
		int adj_adj = msb_dst - dst2->_prec;
		exp_adj1 -= adj_adj;
		exp_adj2 -= adj_adj;
	}
	if ((exp_adj1 < 0 && !_tt_dec_is_true_zero(src1)) ||
			(exp_adj2 < 0 && !_tt_dec_is_true_zero(src2)))
		ret = TT_APN_EROUNDED;

	/* Set result exponent, may adjust later */
	dst2->_exp = src1->_exp - exp_adj1;

	/* Use rounding guard digits to gain precision */
	int adj_adj = 0;
	if (exp_adj2 < 0) {
		int guard_adj = -exp_adj2;
		if (guard_adj > TT_DEC_PREC_RND)
			guard_adj = TT_DEC_PREC_RND;
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
	if (_tt_dec_is_zero(src1))
		exp_adj1 = 0;
	if (_tt_dec_is_zero(src2))
		exp_adj2 = 0;

	/* Copy adjusted significand of src1 to dst */
	tt_assert_fa(dst2->_prec_full > (src1->_msb + exp_adj1));
	shift_digs(dst2->_dig32, dst2->_digsz, src1->_dig32, src1->_msb, exp_adj1);

	/* Copy adjusted significand of src2 to temporary buffer */
	uint *tmpdig = malloc(dst2->_digsz);
	if (!tmpdig) {
		if (dst2 != dst)
			free(dst2);
		return TT_ENOMEM;
	}
	memset(tmpdig, 0, dst2->_digsz);
	tt_assert_fa(dst2->_prec_full > (src2->_msb + exp_adj2));
	shift_digs(tmpdig, dst2->_digsz, src2->_dig32, src2->_msb, exp_adj2);

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
		if (_tt_round(_tt_dec_get_dig(dst2->_dig32, adj_adj) & 1,
				_tt_dec_get_dig(dst2->_dig32, adj_adj-1), 0)) {
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
		dst2->_exp++;
		ret = TT_APN_EROUNDED;
	}
	shift_digs(dst2->_dig32, dst2->_digsz, dst2->_dig32, msb, -adj_adj);

	free(tmpdig);

	/* Switch buffer if dst overlap with src */
	if (dst2 != dst) {
		free(dst->_dig32);
		memcpy(dst, dst2, sizeof(struct tt_dec));
		free(dst2);
	}

	return ret;
}

/* dst = src1 + src2. dst may share src1 or src2. */
int tt_dec_add(struct tt_dec *dst, const struct tt_dec *src1,
		const struct tt_dec *src2)
{
	return add_sub_dec(dst, src1, src2, 0);
}

/* dst = src1 - src2. dst may share src1 or src2. */
int tt_dec_sub(struct tt_dec *dst, const struct tt_dec *src1,
		const struct tt_dec *src2)
{
	return add_sub_dec(dst, src1, src2, 1);
}

/* dst = src1 * src2. dst may share src1 or src2. */
int tt_dec_mul(struct tt_dec *dst, const struct tt_dec *src1,
		const struct tt_dec *src2)
{
	int ret = 0;

	dst->_sign = src1->_sign ^ src2->_sign;

	/* Check NaN, Inf */
	if (src1->_inf_nan == TT_DEC_NAN || src2->_inf_nan == TT_DEC_NAN) {
		dst->_inf_nan = TT_DEC_NAN;
		return TT_APN_EINVAL;
	}
	if (src1->_inf_nan == TT_DEC_INF || src2->_inf_nan == TT_DEC_INF) {
		dst->_inf_nan = TT_DEC_INF;
		return TT_APN_EOVERFLOW;
	}
	dst->_inf_nan = 0;

	/* Check zero */
	if (_tt_dec_is_zero(src1) || _tt_dec_is_zero(src2)) {
		if (src1->_exp == 0 || src2->_exp == 0) {
			dst->_exp = 0;	/* True zero */
		} else {
			dst->_exp = src1->_exp + src1->_msb - 1 +
				src2->_exp + src2->_msb - 1;
			if (dst->_exp > 0)
				dst->_exp = 0;
		}
		dst->_msb = 1;
		memset(dst->_dig32, 0, dst->_digsz);
		return 0;
	}

	dst->_exp = src1->_exp + src2->_exp;

	/* Allocate result buffer
	 * - add one extra rounding guard digits
	 * - add 18 digits for extra 0 introduced by uint boundary
	 */
	const int uints = (src1->_msb + src2->_msb + 1 + 18 + 8) / 9;
	uint *digr = calloc(uints, 4);
	if (!digr)
		return TT_ENOMEM;

	/* Multiply */
	int msb = mul_digs(digr, src1->_dig32, src1->_msb,
			src2->_dig32, src2->_msb);

	/* Adjust significand, msb */
	int adj = 0;
	if (msb > dst->_prec) {
		adj = msb - dst->_prec;

		/* Check rounding */
		if (_tt_round(_tt_dec_get_dig(digr, adj) & 1,
				_tt_dec_get_dig(digr, adj-1), 0)) {
			/* Shift digr to have 1~9 extra digs */
			if (adj >= 10) {
				int shift = (adj - 1) / 9;
				shift *= 9;
				shift_digs(digr, uints*4, digr, msb, -shift);
				adj -= shift;
				msb -= shift;
				dst->_exp += shift;
			}
			/* Add 10^adj */
			tt_assert_fa(adj >=1 && adj <= 9);
			uint one[2] = { 0, 0 };
			one[adj / 9] = one_tbl[adj % 9];
			msb = add_digs(digr, msb, one, adj+1);
			/* Get new exponent adjust */
			adj = msb - dst->_prec;
			tt_assert_fa(adj >= 1);
		}

		dst->_exp += adj;
		dst->_msb = dst->_prec;
		ret = TT_APN_EROUNDED;
	} else {
		dst->_msb = msb;
	}

	/* Copy to dst */
	memset(dst->_dig32, 0, dst->_digsz);
	shift_digs(dst->_dig32, dst->_digsz, digr, msb, -adj);

	free(digr);
	return ret;
}

/* dst = src1 / src2. dst may share src1 or src2. */
int tt_dec_div(struct tt_dec *dst, const struct tt_dec *src1,
		const struct tt_dec *src2)
{
	int ret = 0, sign = src1->_sign ^ src2->_sign;

	dst->_sign = sign;

	/* Check NaN, Inf */
	if (src1->_inf_nan == TT_DEC_NAN || src2->_inf_nan == TT_DEC_NAN) {
		dst->_inf_nan = TT_DEC_NAN;
		return TT_APN_EINVAL;
	}
	if (src1->_inf_nan == TT_DEC_INF) {
		if (src2->_inf_nan == TT_DEC_INF) {
			dst->_inf_nan = TT_DEC_NAN;
			return TT_APN_EINVAL;
		} else {
			dst->_inf_nan = TT_DEC_INF;
			return TT_APN_EOVERFLOW;
		}
	}
	if (src2->_inf_nan == TT_DEC_INF) {
		_tt_dec_zero(dst);
		return 0;
	}

	/* Check zero */
	if (_tt_dec_is_zero(src2)) {
		if (_tt_dec_is_zero(src1)) {
			dst->_inf_nan = TT_DEC_NAN;
			return TT_APN_EDIV_UNDEF;
		} else {
			dst->_inf_nan = TT_DEC_INF;
			return TT_APN_EDIV_0;
		}
	}
	if (_tt_dec_is_zero(src1)) {
		if (src1->_exp == 0) {
			dst->_exp = 0;	/* True zero */
		} else {
			dst->_exp = (src1->_exp + src1->_msb) -
				(src2->_exp + src2->_msb);
			if (dst->_exp > 0)
				dst->_exp = 0;
		}
		dst->_inf_nan = 0;
		dst->_msb = 1;
		memset(dst->_dig32, 0, dst->_digsz);
		return 0;
	}

	/* Allocate a new DEC if dst overlaps src */
	struct tt_dec *quotient = dst;
	if (quotient == src1 || quotient == src2) {
		quotient = tt_dec_alloc(quotient->_prec);
		if (!quotient)
			return TT_ENOMEM;
	} else {
		_tt_dec_zero(quotient);
	}

	quotient->_sign = sign;
	quotient->_exp = src1->_exp - src2->_exp;

	/* Allocate dividend, divisor buffer */
	const int uints = (_tt_max(src1->_msb, src2->_msb) + 1 + 8) / 9;
	uint *dividend = calloc(uints, 4);
	uint *divisor = calloc(uints, 4);
	if (!dividend || !divisor) {
		ret = TT_ENOMEM;
		goto out;
	}
	memcpy(dividend, src1->_dig32, _tt_min(src1->_digsz, uints*4));
	memcpy(divisor, src2->_dig32, _tt_min(src2->_digsz, uints*4));

	int msb_dividend = src1->_msb;
	int msb_divisor = src2->_msb;

	/* Get highest digit */
	int top_dividend = _tt_dec_get_dig(dividend, msb_dividend-1);
	int top_divisor = _tt_dec_get_dig(divisor, msb_divisor-1);
	tt_assert_fa(top_divisor && top_dividend);

	/* Make dividend/divisor within [1, 10) */
	int adj = msb_dividend - msb_divisor;
	quotient->_exp += adj;
	if (adj < 0)
		msb_dividend = shift_digs(dividend, 0,
				dividend, msb_dividend, -adj);
	else if (adj > 0)
		msb_divisor = shift_digs(divisor, 0, divisor, msb_divisor, adj);
	if (cmp_digs(dividend, msb_dividend, divisor, msb_divisor) < 0) {
		quotient->_exp--;
		msb_dividend = shift_digs(dividend, 0, dividend, msb_dividend, 1);
		tt_assert_fa(msb_dividend == msb_divisor + 1);
	} else {
		tt_assert_fa(msb_dividend == msb_divisor);
	}

	/* Remove common trailing zeros */
	while (1) {
		if (msb_dividend <= 9 || msb_divisor <= 9)
			break;
		if (dividend[0] || divisor[0])
			break;
		const int sz_dividend = (msb_dividend + 8) / 9 * 4;
		const int sz_divisor = (msb_divisor + 8) / 9 * 4;
		msb_dividend = shift_digs(dividend, sz_dividend,
				dividend, msb_dividend, -9);
		msb_divisor = shift_digs(divisor, sz_divisor,
				divisor, msb_divisor, -9);
	}

	int msb_result = 1;
	uint *result = quotient->_dig32;
	while (1) {
		tt_assert_fa(msb_result <= (quotient->_prec + 1));

		/* Repeat sub divisor from dividend till remainder < divisor */
		int r;
		for (r = 1; r < 10; r++) {
			msb_dividend = sub_digs(dividend, dividend, msb_dividend,
					divisor, msb_divisor);
			if ((msb_dividend < msb_divisor) ||
					(msb_dividend == msb_divisor &&
					 cmp_digs(dividend, msb_dividend,
						 divisor, msb_divisor) < 0))
				break;
		}
		tt_assert_fa(r < 10 && cmp_digs(dividend, msb_dividend,
					divisor, msb_divisor) < 0);

		/* Store quotient */
		tt_assert_fa(result[0] % 10 == 0);
		result[0] += r;

		/* Precision+rounding reached or residue is zero */
		if ((msb_result > quotient->_prec) ||
				(msb_dividend == 1 && dividend[0] == 0))
			break;

		/* Get highest digit */
		top_dividend = _tt_dec_get_dig(dividend, msb_dividend-1);
		top_divisor = _tt_dec_get_dig(divisor, msb_divisor-1);
		tt_assert_fa(top_divisor && top_dividend);

		/* Make dividend/divisor within [1, 10)
		 * - dividend is less than divisor now
		 */
		adj = msb_dividend - msb_divisor;
		if (adj < 0)
			msb_dividend = shift_digs(dividend, 0,
					dividend, msb_dividend, -adj);
		if (cmp_digs(dividend, msb_dividend, divisor, msb_divisor) < 0) {
			adj--;
			msb_dividend = shift_digs(dividend, 0,
					dividend, msb_dividend, 1);
			tt_assert_fa(msb_dividend == msb_divisor + 1);
		} else {
			tt_assert_fa(msb_dividend == msb_divisor);
		}
		tt_assert_fa(adj < 0);
		adj = -adj;

		/* Append zero to quotient */
		int prec_left = quotient->_prec - msb_result;
		if (adj > (prec_left+1)) {
			/* Precision+rounding reached */
			msb_result = shift_digs(result, 0,
					result, msb_result, prec_left);
			quotient->_exp -= prec_left;
			ret = TT_APN_EROUNDED;
			break;
		}
		msb_result = shift_digs(result, 0, result, msb_result, adj);
		quotient->_exp -= adj;
	}

	/* Check rounding */
	if (msb_result > quotient->_prec) {
		if (_tt_round(_tt_dec_get_dig(result, 1) & 1,
				_tt_dec_get_dig(result, 0), 0)) {
			const uint ten = 10;
			msb_result = add_digs(result, msb_result, &ten, 2);
		}
		adj = msb_result - quotient->_prec;
		msb_result = shift_digs(result, quotient->_digsz,
				result, msb_result, -adj);
		quotient->_exp += adj;
		ret = TT_APN_EROUNDED;
	}

	quotient->_msb = msb_result;

	/* Switch buffer if dst overlaps src */
	if (quotient != dst) {
		free(dst->_dig32);
		memcpy(dst, quotient, sizeof(struct tt_dec));
		free(quotient);
		quotient = NULL;
	}

out:
	if (dividend)
		free(dividend);
	if (divisor)
		free(divisor);
	if (quotient && quotient != dst)
		tt_dec_free(quotient);

	return ret;
}

/* Compare abs
 * TODO: Check NaN, Inf
 * - return < 0: src1 < src2
 * - return = 0: src1 = src2
 * - return > 0: src1 > src2
 */
int tt_dec_cmp_abs(const struct tt_dec *src1, const struct tt_dec *src2)
{
	int msb1 = src1->_msb, msb2 = src2->_msb;

	int ret = (msb1 + src1->_exp) - (msb2 + src2->_exp);
	if (ret)
		return ret;

	uint *dig1 = src1->_dig32, *dig2 = src2->_dig32;
	const int uints = (_tt_max(msb1, msb2) + 8) / 9 * 4;

	/* Align exponent */
	int adj = src1->_exp - src2->_exp;
	if (adj < 0) {
		dig2 = calloc(uints, 4);
		msb2 = shift_digs(dig2, 0, src2->_dig32, msb2, -adj);
	} else if (adj > 0) {
		dig1 = calloc(uints, 4);
		msb1 = shift_digs(dig1, 0, src1->_dig32, msb1, adj);
	}
	tt_assert_fa(msb1 == msb2);

	ret = cmp_digs(dig1, msb1, dig2, msb2);

	if (dig1 != src1->_dig32)
		free(dig1);
	if (dig2 != src2->_dig32)
		free(dig2);

	return ret;
}

int tt_dec_cmp(const struct tt_dec *src1, const struct tt_dec *src2)
{
	int ret;

	if (src1->_sign != src2->_sign) {
		if (_tt_dec_is_zero(src1) && _tt_dec_is_zero(src2))
			return 0;
		ret = src2->_sign - src1->_sign;
	} else {
		ret = tt_dec_cmp_abs(src1, src2);
		if (src1->_sign)
			ret = -ret;
	}

	return ret;
}
