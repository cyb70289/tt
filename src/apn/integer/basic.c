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

#ifdef _TT_LP64_
#define KARA_CROSS	16	/* Karatsuba multiplication cross point */
#define BINDIV_CROSS	30	/* Divide and conquer division cross point */
#else
#define KARA_CROSS	24
#define BINDIV_CROSS	35
#endif

/* Add 31/63 bit integers with carry */
static inline _tt_word add_int(_tt_word i1, _tt_word i2, int *carry)
{
	_tt_word r = i1 + i2 + *carry;

	*carry = !!(r & _tt_word_top_bit);

	return r & ~_tt_word_top_bit;
}

/* Sub 31/63 bit integers with borrow */
static inline _tt_word sub_int(_tt_word i1, _tt_word i2, int *borrow)
{
	_tt_word r = i1 - i2 - *borrow;

	*borrow = !!(r & _tt_word_top_bit);

	return r & ~_tt_word_top_bit;
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
	if (src1->msb < src2->msb)
		__tt_swap(src1, src2);

	/* Make sure dst buffer is large enough */
	if (dst->_max <= src1->msb) {
		ret = _tt_int_realloc(dst, src1->msb+1);
		if (ret)
			return ret;
	}
	dst->msb = src1->msb;

	/* Add uint array */
	int i, carry = 0;
	for (i = 0; i < src2->msb; i++)
		dst->buf[i] = add_int(src1->buf[i], src2->buf[i], &carry);
	for (; i < src1->msb; i++) {
		if (carry == 0) {
			if (dst != src1)
				memcpy(&dst->buf[i], &src1->buf[i],
						(src1->msb-i)*_tt_word_sz);
			return 0;
		}
		dst->buf[i] = src1->buf[i] + 1;
		carry = !dst->buf[i];
	}
	if (carry)
		dst->buf[dst->msb++] = 1;

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
	if (dst->_max < src1->msb) {
		int ret = _tt_int_realloc(dst, src1->msb);
		if (ret)
			return ret;
	}

	/* Sub uint array */
	int i, borrow = 0;
	for (i = 0; i < src2->msb; i++)
		dst->buf[i] = sub_int(src1->buf[i], src2->buf[i], &borrow);
	for (; i < src1->msb; i++) {
		if (borrow == 0) {
			if (dst != src1)
				memcpy(&dst->buf[i], &src1->buf[i],
						(src1->msb-i)*_tt_word_sz);
			break;
		}
		dst->buf[i] = src1->buf[i] - 1;
		borrow = !src1->buf[i];
	}

	/* Check new msb */
	dst->msb = src1->msb;
	while (dst->msb > 1 && dst->buf[dst->msb-1] == 0)
		dst->msb--;

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

	int sign1 = src1->sign;
	int sign2 = src2->sign;
	if (sub)
		sign2 = !sign2;

	if (sign1 == sign2) {
		dst->sign = sign1;
		ret = add_ints_abs(dst, src1, src2);
	} else {
		int cmp12 = tt_int_cmp_abs(src1, src2);
		if (cmp12 >= 0) {
			dst->sign = sign1;
			ret = sub_ints_abs(dst, src1, src2);
		} else {
			dst->sign = sign2;
			ret = sub_ints_abs(dst, src2, src1);
		}
	}

	return ret;
}

/* int1 += int2
 * - int1 must have enough space to hold result
 * - return result msb
 */
int _tt_int_add_buf(_tt_word *int1, int msb1, const _tt_word *int2, int msb2)
{
	int i, carry = 0;

	for (i = 0; i < msb2; i++)
		int1[i] = add_int(int1[i], int2[i], &carry);
	for (; i < msb1; i++) {
		if (carry == 0)
			return msb1;
		int1[i]++;
		carry = !int1[i];
	}
	if (carry)
		int1[i++] = 1;

	return i;
}

/* int1 -= int2
 * - int1 >= int2
 * - return result msb
 */
int _tt_int_sub_buf(_tt_word *int1, int msb1, const _tt_word *int2, int msb2)
{
	int i, borrow = 0;

	for (i = 0; i < msb2; i++)
		int1[i] = sub_int(int1[i], int2[i], &borrow);
	for (; i < msb1; i++) {
		if (borrow == 0)
			return msb1;
		borrow = !int1[i];
		int1[i]--;
	}
	while (i > 1 && int1[i-1] == 0)
		i--;

	return i;
}

/* 1: src1 > src2, 0: src1 == src2, -1: src1 < src2 */
int _tt_int_cmp_buf(const _tt_word *int1, int msb1,
		const _tt_word *int2, int msb2)
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

static int get_words(const _tt_word *ui, int len)
{
	while (len) {
		if (ui[len-1])
			return len;
		len--;
	}
	return 0;
}

int _tt_int_get_msb(const _tt_word *ui, int len)
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
 * - loop unroll on 31 bit word
 */
static int mul_buf_classic(_tt_word *intr, const _tt_word *int1, int msb1,
		const _tt_word *int2, int msb2)
{
	_tt_word c, *r = intr;
	_tt_word_double t0, m;
#ifndef _TT_LP64_
	_tt_word_double t1, t2, t3;
#endif

	/* Square can be faster */
	if (_tt_unlikely(int1 == int2 && msb1 == msb2)) {
		for (int i = 0; i < msb1; i++) {
			t0 = (_tt_word_double)int1[i]*int1[i] + r[i];
			r[i] = t0 & ~_tt_word_top_bit;
			c = t0 >> _tt_word_bits;
			m = int1[i] << 1;

			int j = i + 1;
#ifndef _TT_LP64_
			for (; j <= msb1-4; j += 4) {
				t0 = m * int1[j] + r[j];
				t1 = m * int1[j+1] + r[j+1];
				t2 = m * int1[j+2] + r[j+2];
				t3 = m * int1[j+3] + r[j+3];

				t0 += c;
				r[j] = t0 & ~_tt_word_top_bit;
				t1 += t0 >> _tt_word_bits;
				r[j+1] = t1 & ~_tt_word_top_bit;
				t2 += t1 >> _tt_word_bits;
				r[j+2] = t2 & ~_tt_word_top_bit;
				t3 += t2 >> _tt_word_bits;
				r[j+3] = t3 & ~_tt_word_top_bit;
				c = t3 >> _tt_word_bits;
			}
#endif
			for (; j < msb1; j++) {
				t0 = m * int1[j] + r[j] + c;
				c = t0 >> _tt_word_bits;
				r[j] = t0 & ~_tt_word_top_bit;
			}
			r[msb1] = c;

			r++;
		}
	} else {
		if (msb1 > msb2) {
			__tt_swap(msb1, msb2);
			__tt_swap(int1, int2);
		}

		for (int i = 0; i < msb1; i++) {
			c = 0;
			m = int1[i];

			int j = 0;
#ifndef _TT_LP64_
			for (; j <= msb2-4; j += 4) {
				t0 = m * int2[j] + r[j];
				t1 = m * int2[j+1] + r[j+1];
				t2 = m * int2[j+2] + r[j+2];
				t3 = m * int2[j+3] + r[j+3];

				t0 += c;
				r[j] = t0 & ~_tt_word_top_bit;
				t1 += t0 >> _tt_word_bits;
				r[j+1] = t1 & ~_tt_word_top_bit;
				t2 += t1 >> _tt_word_bits;
				r[j+2] = t2 & ~_tt_word_top_bit;
				t3 += t2 >> _tt_word_bits;
				r[j+3] = t3 & ~_tt_word_top_bit;
				c = t3 >> _tt_word_bits;
			}
#endif
			for (; j < msb2; j++) {
				t0 = m * int2[j] + r[j] + c;
				c = t0 >> _tt_word_bits;
				r[j] = t0 & ~_tt_word_top_bit;
			}
			r[msb2] = c;

			r++;
		}
	}

	/* Top int may be 0 */
	int msb = msb1 + msb2;
	if (intr[msb-1] == 0)
		msb--;

	return msb;
}

static int mul_buf_kara(_tt_word *intr, const _tt_word *int1, int msb1,
		const _tt_word *int2, int msb2, _tt_word *workbuf);

/* Split int2 into pieces of same length as int1 to use Karatsuba efficiently
 * - msb2 >= msb1 * 2
 * - msb1 >= KARA_CROSS
 * - intr = int1 * int2
 * - int1, int2 are not zero
 * - intr must have enough space to hold result
 * - intr is zeroed
 * - return result length
 */
static int mul_buf_unbalanced(_tt_word *intr, const _tt_word *int1, int msb1,
		const _tt_word *int2, int msb2, _tt_word *workbuf)
{
	int msb = 0, msbr = 0, left = msb2, tmpmsb;
	const int tmpsz = msb1 * 2;

	for (int i = 0; i < (msb2+msb1-1)/msb1-1; i++) {
		int words2 = get_words(int2, msb1);
		if (words2) {
			memset(workbuf, 0, tmpsz*_tt_word_sz);
			tmpmsb = mul_buf_kara(workbuf, int1, msb1,
					int2, words2, workbuf+tmpsz);
			msbr = _tt_int_add_buf(intr, msbr, workbuf, tmpmsb)
				- msb1;
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
	memset(workbuf, 0, tmpsz*_tt_word_sz);
	tmpmsb = mul_buf_kara(workbuf, int1, msb1, int2, left, workbuf+tmpsz);
	msbr = _tt_int_add_buf(intr, msbr, workbuf, tmpmsb);

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
static int mul_buf_kara(_tt_word *intr, const _tt_word *int1, int msb1,
		const _tt_word *int2, int msb2, _tt_word *workbuf)
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
	 * algorithm. Fix it by splitting long input into pieces of the same
	 * size as short input.
	 */
	if (msb2 >= msb1*2)
		return mul_buf_unbalanced(intr, int1, msb1, int2, msb2,
				workbuf);

	const int square = (int1 == int2 && msb1 == msb2);

	const int div = msb2 / 2;
	int msb_a = msb1 - div;
	int msb_b = get_words(int1, div);
	int msb_c = msb2 - div;
	int msb_d = get_words(int2, div);

	/* Result = (A*C)*base^(div*2) + (A*D+B*C)*base^div + B*D */
	int msb, msb_ac, msb_bd = 0;

	/* B*D -> intr */
	if (msb_b && msb_d)
		msb_bd = mul_buf_kara(intr, int1, msb_b, int2, msb_d, workbuf);

	/* A*C -> intr+div*2 */
	msb_ac = mul_buf_kara(intr+div*2, int1+div, msb_a, int2+div, msb_c,
			workbuf);
	msb = msb_ac + div*2;

	/* A*D+B*C */
	if (msb_b || msb_d) {
		_tt_word *a_b = workbuf;
		_tt_word *c_d = a_b + (div+2);
		_tt_word *ad_bc = c_d + (div+2);
		_tt_word *nextbuf = ad_bc + 2*(div+2);
		int msb_ad_bc;
		memset(workbuf, 0, 4*(div+2)*_tt_word_sz);

		if (_tt_unlikely(square)) {
			/* Square: A*D*2 */
			msb_ad_bc = mul_buf_kara(ad_bc, int1+div, msb_a,
					int2, msb_d, nextbuf);
			int c = 0;
			for (int i = 0; i < msb_ad_bc; i++) {
				ad_bc[i] <<= 1;
				ad_bc[i] += c;
				if (ad_bc[i] & _tt_word_top_bit) {
					ad_bc[i] -= _tt_word_top_bit;
					c = 1;
				} else {
					c = 0;
				}
			}
			if (c)
				ad_bc[msb_ad_bc++] = 1;
		} else {
			/* A+B */
			int msb_a_b = msb_a;
			memcpy(a_b, int1+div, msb_a*_tt_word_sz);
			if (msb_b)
				msb_a_b = _tt_int_add_buf(a_b, msb_a_b,
						int1, msb_b);

			/* C+D */
			int msb_c_d = msb_c;
			memcpy(c_d, int2+div, msb_c*_tt_word_sz);
			if (msb_d)
				msb_c_d = _tt_int_add_buf(c_d, msb_c_d,
						int2, msb_d);

			/* A*D+B*C = (A+B)*(C+D)-A*C-B*D */
			msb_ad_bc = mul_buf_kara(ad_bc, a_b, msb_a_b,
					c_d, msb_c_d, nextbuf);
			msb_ad_bc = _tt_int_sub_buf(ad_bc, msb_ad_bc,
					intr+div*2, msb_ac);
			if (msb_bd)
				msb_ad_bc = _tt_int_sub_buf(ad_bc, msb_ad_bc,
						intr, msb_bd);
		}

		/* Sum all */
		int msb2 = msb - div;
		msb = _tt_int_add_buf(intr+div, msb2, ad_bc, msb_ad_bc) + div;
	}

	tt_assert_fa(msb == (msb1+msb2) || msb == (msb1+msb2-1));
	return msb;
}

/* intr = int1 * int2
 * - intr is zeroed on enter
 * - return result length, or TT_ENOMEM
 */
int _tt_int_mul_buf(_tt_word *intr, const _tt_word *int1, int msb1,
		const _tt_word *int2, int msb2)
{
	/* mul_buf_xxx cannot treat 0 correctly */
	if ((msb1 == 1 && int1[0] == 0) || (msb2 == 1 && int2[0] == 0))
		return 1;

	int msbmin, msbmax;
	if (msb1 > msb2) {
		msbmax = msb1;
		msbmin = msb2;
	} else {
		msbmax = msb2;
		msbmin = msb1;
	}

	/* Use classic algorithm when input size below crosspoint */
	if (msbmin < KARA_CROSS)
		return mul_buf_classic(intr, int1, msb1, int2, msb2);

	/* Allocate working buffer for Karatsuba algorithm */
	/* recursive calls = (int)(log2(msbmax) - log2(KARA_CROSS)) + 2; */
	const int recurse = 33 - __builtin_clz((msbmax / KARA_CROSS) + 1);
	const int worksz = msbmax * 4 + recurse * (recurse + 9);
	void *workbuf = malloc(worksz * _tt_word_sz);
	if (workbuf == NULL)
		return TT_ENOMEM;

	int ret = mul_buf_kara(intr, int1, msb1, int2, msb2, workbuf);

	free(workbuf);
	return ret;
}

/* Guess top digit of src1/src2
 * - return src1[1][0] / src2[0]
 * - src2 is normalized
 */
static _tt_word guess_quotient(const _tt_word *src1, int msb1,
		const _tt_word *src2, int msb2)
{
	if (_tt_int_cmp_buf(src1, msb1, src2, msb2) < 0)
		return 0;
	else if (msb1 == msb2)
		return 1;

	_tt_word_double dividend = src1[msb1-1];
	dividend <<= _tt_word_bits;
	dividend |= src1[msb1-2];

	_tt_word q = (_tt_word)(dividend / src2[msb2-1]);
	tt_assert_fa(q <= _tt_word_top_bit);
	return q;
}

/* Paramaters same as _tt_int_div_buf() */
static int div_buf_classic(_tt_word *qt, int *msb_qt, _tt_word *rm, int *msb_rm,
		const _tt_word *dd, int msb_dd, const _tt_word *ds, int msb_ds)
{
	/* Allocate working buffer */
	const int sz_tmpmul = msb_ds + 1;
	_tt_word *workbuf = malloc((msb_dd+sz_tmpmul)*_tt_word_sz);
	if (workbuf == NULL)
		return TT_ENOMEM;
	_tt_word *_dd = workbuf;
	_tt_word *tmpmul = _dd + msb_dd;

	memcpy(_dd, dd, msb_dd*_tt_word_sz);

	int qt_idx = msb_dd - msb_ds - 1, topmsb = msb_ds;
	_tt_word *dd_top = _dd + msb_dd - msb_ds;

	*msb_qt = msb_dd - msb_ds;
	if (_tt_int_cmp_buf(dd_top, msb_ds, ds, msb_ds) >= 0) {
		/* First digit */
		qt[(*msb_qt)++] = 1;
		topmsb = _tt_int_sub_buf(dd_top, msb_ds, ds, msb_ds);
	}
	if (*msb_qt == 0)
		*msb_qt = 1;

	while (qt_idx >= 0) {
		if (!(topmsb == 1 && *dd_top == 0))
			topmsb++;
		dd_top--;

		_tt_word q = guess_quotient(dd_top, topmsb, ds, msb_ds);
		if (q) {
			memset(tmpmul, 0, sz_tmpmul*_tt_word_sz);
			int mulmsb = mul_buf_classic(tmpmul, ds, msb_ds, &q, 1);

			while (_tt_int_cmp_buf(tmpmul, mulmsb, dd_top, topmsb)
					> 0) {
				mulmsb = _tt_int_sub_buf(tmpmul, mulmsb,
						ds, msb_ds);
				q--;
			}
			tt_assert_fa(topmsb >= mulmsb);
			topmsb = _tt_int_sub_buf(dd_top, topmsb,
					tmpmul, mulmsb);
		}
		qt[qt_idx] = q;
		qt_idx--;
	}
	tt_assert_fa(dd_top == _dd);

	/* Remainder */
	tt_assert_fa(topmsb <= msb_ds);
	memcpy(rm, _dd, topmsb*_tt_word_sz);
	*msb_rm = topmsb;

	free(workbuf);
	return 0;
}

/* Divide and conquer division
 * - Paramaters same as _tt_int_div_buf()
 * - msb_dd <= msb_ds*2
 */
static int div_buf_bin(_tt_word *qt, int *msb_qt, _tt_word *rm, int *msb_rm,
		const _tt_word *dd, int msb_dd, const _tt_word *ds, int msb_ds)
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
	_tt_word *workbuf = calloc(bufsz, _tt_word_sz);
	if (workbuf == NULL)
		return TT_ENOMEM;
	_tt_word *nextdd = workbuf, *nextdd_rm = nextdd + h*2;
	_tt_word *qt_ds = nextdd + msb_ds + h + 1;
	_tt_word *qtl = qt_ds + h*2 + 2;

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
	_tt_word *qth = qt + h;

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
	memcpy(nextdd, dd, h*2*_tt_word_sz);
	int msb_nextdd = _tt_int_get_msb(nextdd, msb_rmh + h*2);
	int msb_qt_ds = _tt_int_mul_buf(qt_ds+h, qth, msb_qth,
			ds, _tt_int_get_msb(ds, h)) + h;
	msb_qt_ds = _tt_int_get_msb(qt_ds, msb_qt_ds);

	while (_tt_int_cmp_buf(nextdd, msb_nextdd, qt_ds, msb_qt_ds) < 0) {
		const _tt_word one = 1;
		msb_qth = _tt_int_sub_buf(qth, msb_qth, &one, 1);
		if (msb_qt_ds > h &&
				_tt_int_cmp_buf(qt_ds+h, msb_qt_ds-h,
					ds, msb_ds) > 0) {
			/* Decrease subtrahend */
			msb_qt_ds = _tt_int_sub_buf(qt_ds+h, msb_qt_ds-h,
					ds, msb_ds) + h;
		} else {
			/* Increase minuend */
			int msb_tmp = msb_nextdd - h;
			if (msb_tmp <= 0)
				msb_tmp = 1;
			msb_nextdd = _tt_int_add_buf(nextdd+h, msb_tmp,
					ds, msb_ds) + h;
			break;
		}
	}
	msb_nextdd = _tt_int_sub_buf(nextdd, msb_nextdd, qt_ds, msb_qt_ds);

	/* Clear used buffer */
	memset(qt_ds+h, 0, (msb_dd-msb_ds+2)*_tt_word_sz);

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
	_tt_word *rml = rm + h;

	if (msb_nextdd > h &&
			_tt_int_cmp_buf(nextdd+h, msb_nextdd-h,
				ds+h, msb_ds-h) >= 0) {
		ret = div_buf_bin(qtl, &msb_qtl, rml, &msb_rml,
				nextdd+h, msb_nextdd-h, ds+h, msb_ds-h);
		if (ret)
			goto out;
	} else {
		msb_qtl = msb_rml = 1;
		if (msb_nextdd > h) {
			msb_rml = msb_nextdd - h;
			memcpy(rml, nextdd+h, msb_rml*_tt_word_sz);
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
	memcpy(rm, nextdd, h*_tt_word_sz);
	msb_rml = _tt_int_get_msb(rm, msb_rml+h);
	msb_qt_ds = _tt_int_mul_buf(qt_ds, qtl, msb_qtl,
			ds, _tt_int_get_msb(ds, h));

	while (_tt_int_cmp_buf(rm, msb_rml, qt_ds, msb_qt_ds) < 0) {
		const _tt_word one = 1;
		msb_qtl = _tt_int_sub_buf(qtl, msb_qtl, &one, 1);
		if (_tt_int_cmp_buf(qt_ds, msb_qt_ds, ds, msb_ds) > 0) {
			/* Decrease subtrahend */
			msb_qt_ds = _tt_int_sub_buf(qt_ds, msb_qt_ds,
					ds, msb_ds);
		} else {
			/* Increase minuend */
			msb_rml = _tt_int_add_buf(rm, msb_rml, ds, msb_ds);
			break;
		}
	}
	*msb_rm = _tt_int_sub_buf(rm, msb_rml, qt_ds, msb_qt_ds);

	*msb_qt = _tt_int_add_buf(qt, msb_qth+h, qtl, msb_qtl);

out:
	free(workbuf);
	return ret;
}

/* Divide: qt = dd / ds; rm = dd % ds
 * - dividend >= divisor
 * - divisor is normalized (if msb_ds > 1)
 * - qt: quotient, zeroed, size = max_quotient_words + 1
 * - rm: remainder, zeroed, size = max_remainder_words + 1
 * - msb_qt/msb_rm: quotient/remainder msb
 */
int _tt_int_div_buf(_tt_word *qt, int *msb_qt, _tt_word *rm, int *msb_rm,
		const _tt_word *dd, int msb_dd, const _tt_word *ds, int msb_ds)
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
	_tt_word *_dd = malloc(msb_ds*2*_tt_word_sz);
	memcpy(_dd, dd+offset, msb_ds*2*_tt_word_sz);

	while (1) {
		/* Partial division */
		int _msb_dd = _tt_int_get_msb(_dd, msb_ds*2);
		if (_tt_int_cmp_buf(_dd, _msb_dd, ds, msb_ds) >= 0) {
			ret = div_buf_bin(qt+offset, &_msb_qt, rm, &_msb_rm,
					_dd, _msb_dd, ds, msb_ds);
			if (ret)
				goto out;
		} else {
			_msb_qt = 1;
			_msb_rm = msb_ds;
			memcpy(rm, _dd, _msb_dd*_tt_word_sz);
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
		memcpy(_dd, dd+offset, (msb_ds*2-_msb_rm)*_tt_word_sz);
		memcpy(_dd+msb_ds*2-_msb_rm, rm, _msb_rm*_tt_word_sz);
		memset(rm, 0, msb_ds*_tt_word_sz);
	}

	/* Last division */
	memcpy(_dd, dd, (msb_dd-_msb_rm)*_tt_word_sz);
	memcpy(_dd+msb_dd-_msb_rm, rm, _msb_rm*_tt_word_sz);
	memset(_dd+msb_dd, 0, (msb_ds*2-msb_dd)*_tt_word_sz);
	msb_dd = _tt_int_get_msb(_dd, msb_dd);
	if (_tt_int_cmp_buf(_dd, msb_dd, ds, msb_ds) >= 0) {
		memset(rm, 0, msb_ds*_tt_word_sz);
		ret = div_buf_bin(qt, &_msb_qt, rm, msb_rm,
				_dd, msb_dd, ds, msb_ds);
	} else {
		memcpy(rm, _dd, msb_dd*_tt_word_sz);
		memset(rm+msb_dd, 0, (msb_ds-msb_dd)*_tt_word_sz);
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
	_tt_word *r = calloc(src1->msb + src2->msb, _tt_word_sz);
	if (!r)
		return TT_ENOMEM;

	const int sign = src1->sign ^ src2->sign;
	int msb = _tt_int_mul_buf(r, src1->buf, src1->msb,
			src2->buf, src2->msb);
	/* TODO: drop memcpy by replacing dst->buf with r directly */
	if (dst->_max < msb) {
		int ret = _tt_int_realloc(dst, msb);
		if (ret)
			return ret;
	}
	memcpy(dst->buf, r, msb*_tt_word_sz);
	memset(dst->buf + msb, 0, (dst->_max - msb)*_tt_word_sz);
	dst->msb = msb;
	dst->sign = sign;

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
	const int sign_quo = src1->sign ^ src2->sign;
	const int sign_rem = src1->sign;

	/* Working buffer for quotient, remainder */
	_tt_word *qt = NULL, *rm = NULL;
	int msb_qt = src1->msb - src2->msb + 3;	/* One word + Shift */
	int msb_rm = src2->msb + 2;		/* " */

	/* Working buffer for normalized dividend, divisor */
	_tt_word *ds = (_tt_word *)src2->buf;
	_tt_word *dd = (_tt_word *)src1->buf;
	int msb_ds = src2->msb;
	int msb_dd = src1->msb;

	/* Get shift bits to normalize divisor */
	int shift = _tt_word_bits - _tt_int_word_bits(ds[msb_ds-1]);

	/* Allocate working buffer */
	_tt_word *workbuf;
	if (shift)
		workbuf = calloc(msb_qt+msb_rm+msb_ds+msb_dd+1, _tt_word_sz);
	else
		workbuf = calloc(msb_qt+msb_rm, _tt_word_sz);
	if (workbuf == NULL)
		return TT_ENOMEM;
	qt = workbuf;
	rm = workbuf + msb_qt;
	if (shift) {
		/* Normalize */
		ds = workbuf + msb_qt + msb_rm;
		dd = ds + msb_ds;

		_tt_word tmp = 0;
		for (int i = 0; i < msb_ds; i++) {
			ds[i] = ((src2->buf[i] << shift) | tmp) &
				~_tt_word_top_bit;
			tmp = src2->buf[i] >> (_tt_word_bits - shift);
		}

		tt_assert(tmp == 0);
		for (int i = 0; i < msb_dd; i++) {
			dd[i] = ((src1->buf[i] << shift) | tmp) &
				~_tt_word_top_bit;
			tmp = src1->buf[i] >> (_tt_word_bits - shift);
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
		quo->sign = sign_quo;
		quo->msb = msb_qt;
		memcpy(quo->buf, qt, msb_qt*_tt_word_sz);
	}

	/* rm[msb_rm] -> remainder */
	if (rem) {
		_tt_int_zero(rem);
		ret = _tt_int_realloc(rem, msb_rm);
		if (ret)
			goto out;
		msb_rm = _tt_int_shift_buf(rm, msb_rm, -shift);
		rem->sign = sign_rem;
		rem->msb = msb_rm;
		memcpy(rem->buf, rm, msb_rm*_tt_word_sz);
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
	return _tt_int_cmp_buf(src1->buf, src1->msb, src2->buf, src2->msb);
}

int tt_int_cmp(const struct tt_int *src1, const struct tt_int *src2)
{
	int ret;

	if (src1->sign != src2->sign) {
		if (_tt_int_is_zero(src1) && _tt_int_is_zero(src2))
			return 0;
		ret = src2->sign - src1->sign;
	} else {
		ret = tt_int_cmp_abs(src1, src2);
		if (src1->sign)
			ret = -ret;
	}

	return ret;
}
