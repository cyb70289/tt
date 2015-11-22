/* Add, sub, mul, div, logic
 *
 * Copyright (C) 2015 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/apn/integer.h>
#include <common/lib.h>
#include "integer.h"

#include <string.h>
#include <math.h>

#define KARA_CROSS	200	/* Karatsuba multiplication cross point */
#define BINDIV_CROSS	9	/* Divide and conquer division cross point */

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

static int get_msb(const uint *ui, int len)
{
	while (len) {
		if (ui[len-1])
			return len;
		len--;
	}
	return 1;
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
		/* XXX: int128 may overflow if both operands >= 10^(10^21) */
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

static int mul_buf_kara(uint *intr, const uint *int1, int msb1,
		const uint *int2, int msb2, uint *workbuf);

/* Split int2 into pieces of same length as int1 to use Karatsuba efficiently
 * - msb2 >= msb1 * 2
 * - msb1 >= KARA_CROSS
 * - intr = int1 * int2
 * - int1, int2 are not zero
 * - intr must have enough space to hold result
 * - intr is zeroed
 * - return result length
 */
static int mul_buf_unbalanced(uint *intr, const uint *int1, int msb1,
		const uint *int2, int msb2, uint *workbuf)
{
	int msb = 0, msbr = 0, left = msb2, tmpmsb;
	const int tmpsz = msb1 * 2;

	for (int i = 0; i < (msb2+msb1-1)/msb1-1; i++) {
		int uints2 = get_uints(int2, msb1);
		if (uints2) {
			memset(workbuf, 0, tmpsz * 4);
			tmpmsb = mul_buf_kara(workbuf, int1, msb1,
					int2, uints2, workbuf + tmpsz);
			msbr = add_buf(intr, msbr, workbuf, tmpmsb) - msb1;
			if (msbr < 0)
				msbr = 0;
		} else {
			msbr = 0;
		}
		intr += msb1;
		int2 += msb1;
		msb += msb1;
		left -= msb1;
	}

	tt_assert_fa(left);
	memset(workbuf, 0, tmpsz * 4);
	tmpmsb = mul_buf_kara(workbuf, int1, msb1, int2, left, workbuf + tmpsz);
	msbr = add_buf(intr, msbr, workbuf, tmpmsb);

	return msb + msbr;
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
static int mul_buf_kara(uint *intr, const uint *int1, int msb1,
		const uint *int2, int msb2, uint *workbuf)
{
	/* Make int1 shorter than int2 */
	if (msb1 > msb2) {
		__tt_swap(msb1, msb2);
		__tt_swap(int1, int2);
	}

	/* Fallback to classic algorithm on small inputs */
	if (msb1 < KARA_CROSS)
		return mul_buf_classic(intr, int1, msb1, int2, msb2);

	/* On unbalanced input, Karatsuba performs even worse than classic
	 * algorithm. Fix it by spliting long input into pieces of the same
	 * size as short input .
	 */
	if (msb2 >= msb1*2)
		return mul_buf_unbalanced(intr, int1, msb1, int2, msb2, workbuf);

	const int div = msb2 / 2;
	int msb_a = msb1 - div;
	int msb_b = get_uints(int1, div);
	int msb_c = msb2 - div;
	int msb_d = get_uints(int2, div);

	/* Result = (A*C)*base^(div*2) + (A*D+B*C)*base^div + B*D */
	int msb, msb_ac, msb_bd = 0;

	/* B*D -> intr */
	if (msb_b && msb_d)
		msb_bd = mul_buf_kara(intr, int1, msb_b, int2, msb_d, workbuf);

	/* A*C -> intr+div*2 */
	msb_ac = mul_buf_kara(intr+div*2, int1+div, msb_a, int2+div, msb_c, workbuf);
	msb = msb_ac + div*2;

	/* A*D+B*C */
	if (msb_b || msb_d) {
		uint *a_b = workbuf;
		uint *c_d = a_b + (div + 2);
		uint *ad_bc = c_d + (div + 2);
		uint *nextbuf = ad_bc + 2 * (div + 2);
		const int bufsz = 4 * (div + 2);
		memset(workbuf, 0, bufsz * 4);

		/* A+B */
		int msb_a_b = msb_a;
		memcpy(a_b, int1+div, msb_a*4);
		if (msb_b)
			msb_a_b = add_buf(a_b, msb_a_b, int1, msb_b);

		/* C+D */
		int msb_c_d = msb_c;
		memcpy(c_d, int2+div, msb_c*4);
		if (msb_d)
			msb_c_d = add_buf(c_d, msb_c_d, int2, msb_d);

		/* A*D+B*C = (A+B)*(C+D)-A*C-B*D */
		int msb_ad_bc = mul_buf_kara(ad_bc, a_b, msb_a_b, c_d, msb_c_d,
				nextbuf);
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
 * - int1, msb1, int2, msb2 must from valid tt_int
 * - intr is zeroed on enter
 * - return result length, -1 if no enough memory
 */
int _tt_int_mul_buf(uint *intr, const uint *int1, int msb1,
		const uint *int2, int msb2)
{
	/* mul_buf_xxx cannot treat 0 correctly */
	if ((msb1 == 1 && int1[0] == 0) || (msb2 == 1 && int2[0] == 0))
		return 1;

	const int msbmax = _tt_max(msb1, msb2);

	/* Use classic algorithm when input size below crosspoint */
	if (msbmax < KARA_CROSS)
		return mul_buf_classic(intr, int1, msb1, int2, msb2);

	/* Allocate working buffer for Karatsuba algorithm */
	/* recursive calls = (int)(log2(msbmax) - log2(KARA_CROSS)) + 2; */
	const int recurse = 33 - __builtin_clz((msbmax / KARA_CROSS) + 1);
	const int worksz = msbmax * 4 + recurse * (recurse + 9);
	uint *workbuf = malloc(worksz * 4);
	if (workbuf == NULL)
		return TT_ENOMEM;

	int ret = mul_buf_kara(intr, int1, msb1, int2, msb2, workbuf);

	free(workbuf);
	return ret;
}

/* Guess quotient by high digits */
static uint guess_quotient(const uint *_src1, int msb1,
		const uint *_src2, int msb2)
{
	if (cmp_buf(_src1, msb1, _src2, msb2) < 0)
		return 0;
	else if (msb1 == msb2)
		return 1;

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
	tt_assert_fa(q < BIT(31));
	return q;
}

/* Paramaters same as _tt_int_div_buf() */
static int div_buf_classic(uint *qt, int *msb_qt, uint *rm, int *msb_rm,
		const uint *dd, int msb_dd, const uint *ds, int msb_ds)
{
	/* Allocate working buffer */
	const int sz_tmpmul = msb_ds + 1;
	uint *workbuf = malloc((msb_dd + sz_tmpmul) * 4);
	if (workbuf == NULL)
		return TT_ENOMEM;
	uint *_dd = workbuf;
	uint *tmpmul = _dd + msb_dd;

	memcpy(_dd, dd, msb_dd * 4);

	int qt_idx = msb_dd - msb_ds - 1, topmsb = msb_ds;
	uint *dd_top = _dd + msb_dd - msb_ds;

	*msb_qt = msb_dd - msb_ds;
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
		qt[qt_idx] = q;
		qt_idx--;
	}
	tt_assert_fa(dd_top == _dd);

	/* remainder */
	tt_assert_fa(topmsb <= msb_ds);
	memcpy(rm, _dd, topmsb * 4);
	*msb_rm = topmsb;

	free(workbuf);
	return 0;
}

/* Divide and conquer division
 * - Paramaters same as _tt_int_div_buf()
 * - msb_dd <= msb_ds*2
 */
static int div_buf_bin(uint *qt, int *msb_qt, uint *rm, int *msb_rm,
		const uint *dd, int msb_dd, const uint *ds, int msb_ds)
{
	int ret = 0;
	const int m = msb_dd - msb_ds, h = m / 2;

	if (m < BINDIV_CROSS)
		return div_buf_classic(qt, msb_qt, rm, msb_rm,
				dd, msb_dd, ds, msb_ds);

	/* Working buffer
	 *
	 * - For high quotient
	 *   +------------------------+-------------------------+
	 *   |         nextdd         |          qt_ds          |
	 *   +------------------------+-------------------------+
	 *   |<----- msb_ds+h+1 ----->|<-- msb_dd-msb_ds+h+2 -->|
	 *
	 * - For low quotient
	 *   +------------------------+----------+---------+
	 *   |         nextdd         |   qt_ds  |   qtl   |
	 *   +------------------------+----------+---------+
	 *   |<----- msb_ds+h+1 ----->|<- 2h+2 ->|<- h+2 ->|
	 */
	const int bufsz = msb_ds + h*4 + 5;
	uint *workbuf = calloc(bufsz, 4);
	if (workbuf == NULL)
		return TT_ENOMEM;
	uint *nextdd = workbuf, *nextdd_rm = nextdd + h*2;
	uint *qt_ds = nextdd + msb_ds + h + 1;
	uint *qtl = qt_ds + h*2 + 2;

	/* Guess higher part quotient
	 *
	 * - Dividend:
	 *   +-----------------+-------------+
	 *   |       dd1       |     dd0     |
	 *   +-----------------+-------------+
	 *   |                 |<--- h*2 --->|
	 *   |<--------- msb_dd ------------>|
	 *
	 * - Divisor:
	 *   +---------+-------+
	 *   |   ds1   |  ds0  |
	 *   +---------+-------+
	 *   |         |<- h ->|
	 *   |<--- msb_ds ---->|
	 *
	 * - (qt, rm) = dd1 .divide. ds1
	 *   qt_max_size = msb_dd - msb_ds - h + 2 (one extra word)
	 *   rm_max_size = msb_ds - h
	 */
	int msb_qth, msb_rmh;
	uint *qth = qt + h;

	ret = div_buf_bin(qth, &msb_qth, nextdd_rm, &msb_rmh,
			dd+h*2, msb_dd-h*2, ds+h, msb_ds-h);
	if (ret)
		goto out;

	/* Adjust quotient (Next dividend >= Quotient*Divisor)
	 *
	 * - Next dividend (max_size = msb_ds + h + 1) (one extra word)
	 *   +------+-------------+
	 *   |  rm  |     dd0     |
	 *   +------+-------------+
	 *          |<--- h*2 --->|
	 *
	 * - Quotient*Divisor (max_size = msb_dd - msb_ds + h + 2)
	 *   +------------+-------+
	 *   |  qt * ds0  |   0   |
	 *   +------------+-------+
	 *                |<- h ->|
	 */
	memcpy(nextdd, dd, h*2*4);
	int msb_nextdd = get_msb(nextdd, msb_rmh + h*2);
	int msb_qt_ds = _tt_int_mul_buf(qt_ds+h, qth, msb_qth,
			ds, get_msb(ds, h)) + h;
	msb_qt_ds = get_msb(qt_ds, msb_qt_ds);

	while (cmp_buf(nextdd, msb_nextdd, qt_ds, msb_qt_ds) < 0) {
		const uint one = 1;
		msb_qth = sub_buf(qth, msb_qth, &one, 1);
		if (msb_qt_ds > h &&
				cmp_buf(qt_ds+h, msb_qt_ds-h, ds, msb_ds) > 0) {
			/* Decrease subtrahend */
			msb_qt_ds = sub_buf(qt_ds+h, msb_qt_ds-h, ds, msb_ds) + h;
		} else {
			/* Increase minuend */
			int msb_tmp = msb_nextdd - h;
			if (msb_tmp <= 0)
				msb_tmp = 1;
			msb_nextdd = add_buf(nextdd+h, msb_tmp, ds, msb_ds) + h;
			break;
		}
	}
	msb_nextdd = sub_buf(nextdd, msb_nextdd, qt_ds, msb_qt_ds);

	/* Clear used buffer */
	memset(qt_ds+h, 0, (msb_dd-msb_ds+2)*4);

	/* Get lower part quotient
	 *
	 * - Next dividend (max_size = msb_ds + h + 1)
	 *   +--------------+----------+
	 *   |   next_dd1   | next_dd0 |
	 *   +--------------+----------+
	 *                  |<-- h --->|
	 *
	 * - (qt, rm) = next_dd1 .divide. ds1
	 *   qt_max_size = h + 2 (one extra word)
	 *   rm_max_size = msb_ds - h + 1 (one extra word)
	 */
	int msb_qtl, msb_rml;
	uint *rml = rm + h;

	if (msb_nextdd > h &&
			cmp_buf(nextdd+h, msb_nextdd-h, ds+h, msb_ds-h) >= 0) {
		ret = div_buf_bin(qtl, &msb_qtl, rml, &msb_rml,
				nextdd+h, msb_nextdd-h, ds+h, msb_ds-h);
		if (ret)
			goto out;
	} else {
		msb_qtl = msb_rml = 1;
		if (msb_nextdd > h) {
			msb_rml = msb_nextdd - h;
			memcpy(rml, nextdd+h, msb_rml*4);
		}
	}

	/* Adjust quotient (Remainder >= Quotient*Divisor)
	 *
	 * - Remainder (maxsize = msb_ds)
	 *   +-------+----------+
	 *   |  rml  | next_dd0 |
	 *   +-------+----------+
	 *           |<-- h --->|
	 *
	 * - Quotient*Divisor (maxize = 2h+2)
	 *   +---------+
	 *   | qtl*ds0 |
	 *   +---------+
	 */
	memcpy(rm, nextdd, h*4);
	msb_rml = get_msb(rm, msb_rml+h);
	msb_qt_ds = _tt_int_mul_buf(qt_ds, qtl, msb_qtl, ds, get_msb(ds, h));

	while (cmp_buf(rm, msb_rml, qt_ds, msb_qt_ds) < 0) {
		const uint one = 1;
		msb_qtl = sub_buf(qtl, msb_qtl, &one, 1);
		if (cmp_buf(qt_ds, msb_qt_ds, ds, msb_ds) > 0) {
			/* Decrease subtrahend */
			msb_qt_ds = sub_buf(qt_ds, msb_qt_ds, ds, msb_ds);
		} else {
			/* Increase minuend */
			msb_rml = add_buf(rm, msb_rml, ds, msb_ds);
			break;
		}
	}
	*msb_rm = sub_buf(rm, msb_rml, qt_ds, msb_qt_ds);

	*msb_qt = add_buf(qt, msb_qth+h, qtl, msb_qtl);

out:
	free(workbuf);
	return ret;
}

/* Divide: qt = dd / ds; rm = dd % ds
 * - dividend >= divisor
 * - divisor is normalized
 * - qt: quotient, zeroed, size = max_quotient_words + 1
 * - rm: remainder, zeroed, size = max_remainder_words
 * - msb_qt/msb_rm: quotient/remainder msb
 */
int _tt_int_div_buf(uint *qt, int *msb_qt, uint *rm, int *msb_rm,
		const uint *dd, int msb_dd, const uint *ds, int msb_ds)
{
	if (msb_ds < BINDIV_CROSS)
		return div_buf_classic(qt, msb_qt, rm, msb_rm,
				dd, msb_dd, ds, msb_ds);

	if (msb_dd <= msb_ds*2)
		return div_buf_bin(qt, msb_qt, rm, msb_rm,
				dd, msb_dd, ds, msb_ds);

	int ret;
	int _msb_qt, _msb_rm = 0;
	int offset = msb_dd - msb_ds*2;

	*msb_qt = 0;	/* Invalidate */

	/* Temporary dividend buffer */
	uint *_dd = malloc(msb_ds*2*4);
	memcpy(_dd, dd+offset, msb_ds*2*4);

	while (1) {
		/* Partial division */
		int _msb_dd = get_msb(_dd, msb_ds*2);
		if (cmp_buf(_dd, _msb_dd, ds, msb_ds) >= 0) {
			ret = div_buf_bin(qt+offset, &_msb_qt, rm, &_msb_rm,
					_dd, _msb_dd, ds, msb_ds);
			if (ret)
				goto out;
		} else {
			_msb_qt = 1;
			_msb_rm = msb_ds;
			memcpy(rm, ds, msb_ds*4);
		}

		/* Calculate quotient MSB on first division */
		if (*msb_qt == 0)
			*msb_qt = _msb_qt + offset;

		/* Get next dividend MSB */
		if (_msb_rm == 1 && rm[0] == 0)
			_msb_rm = 0;
		msb_dd = offset + _msb_rm;
		offset -= (msb_ds*2 - _msb_rm);
		if (offset <= 0)
			break;

		/* Combine remainder to next dividend */
		memcpy(_dd, dd+offset, (msb_ds*2-_msb_rm)*4);
		memcpy(_dd+msb_ds*2-_msb_rm, rm, _msb_rm*4);
		memset(rm, 0, msb_ds*4);
	}

	/* Last division */
	tt_assert(msb_dd <= msb_ds*2);
	memcpy(_dd, dd, (msb_dd-_msb_rm)*4);
	memcpy(_dd+msb_dd-_msb_rm, rm, _msb_rm*4);
	memset(_dd+msb_dd, 0, (msb_ds*2-msb_dd)*4);
	msb_dd = get_msb(_dd, msb_dd);
	if (cmp_buf(_dd, msb_dd, ds, msb_ds) >= 0) {
		memset(rm, 0, msb_ds*4);
		ret = div_buf_bin(qt, &_msb_qt, rm, msb_rm,
				_dd, msb_dd, ds, msb_ds);
	} else {
		memcpy(rm, _dd, msb_dd*4);
		memset(rm+msb_dd, 0, (msb_ds-msb_dd)*4);
		*msb_rm = msb_dd;
		ret = 0;
	}

out:
	free(_dd);
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

	/* Get shift bits to normalize divisor */
	const int shift = __builtin_clz(src2->_int[src2->_msb-1]) - 1;

	/* Working buffer for quotient, remainder */
	uint *qt = NULL, *rm = NULL;
	int msb_qt = src1->_msb - src2->_msb + 2;	/* one extra word */
	int msb_rm = src2->_msb + 1;			/* " */

	/* Working buffer for normalized dividend, divisor */
	uint *ds = (uint *)src2->_int;
	uint *dd = (uint *)src1->_int;
	int msb_ds = src2->_msb;
	int msb_dd = src1->_msb;

	/* Allocate working buffer */
	uint *workbuf;
	if (shift)
		workbuf = calloc(msb_qt + msb_rm + msb_ds + msb_dd + 1, 4);
	else
		workbuf = calloc(msb_qt + msb_rm, 4);
	if (workbuf == NULL)
		return TT_ENOMEM;
	qt = workbuf;
	rm = workbuf + msb_qt;
	if (shift) {
		/* Normalize */
		ds = workbuf + msb_qt + msb_rm;
		dd = ds + msb_ds;

		uint tmp = 0;
		for (int i = 0; i < msb_ds; i++) {
			ds[i] = ((src2->_int[i] << shift) | tmp) & ~BIT(31);
			tmp = src2->_int[i] >> (31 - shift);
		}

		tt_assert(tmp == 0);
		for (int i = 0; i < msb_dd; i++) {
			dd[i] = ((src1->_int[i] << shift) | tmp) & ~BIT(31);
			tmp = src1->_int[i] >> (31 - shift);
		}
		if (tmp)
			dd[msb_dd++] = tmp;
	}

	/* Do division */
	ret = _tt_int_div_buf(qt, &msb_qt, rm, &msb_rm, dd, msb_dd, ds, msb_ds);
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
		if (shift) {
			uint tmp = 0, tmp2;
			for (int i = msb_rm-1; i >= 0; i--) {
				tmp2 = rm[i];
				rm[i] = (rm[i] >> shift) | tmp;
				tmp = (tmp2 << (31 - shift)) & ~BIT(31);
			}
			if (rm[msb_rm-1] == 0 && msb_rm > 1)
				msb_rm--;
		}
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
