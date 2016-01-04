/* Primality testing
 *
 * Copyright (C) 2015 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/apn/integer.h>
#include <common/lib.h>
#include "integer.h"

#include "string.h"

/* Naive algorithm (p < 2^31) */
static bool isprime_naive(uint p)
{
	if (p == 2 || p == 3)
		return true;
	if (p < 2 || p % 2 == 0 || p % 3 == 0)
		return false;

	uint i = 5;
	while (i*i <= p) {
		if (p % i == 0 || p % (i+2) == 0)
			return false;
		i += 6;
	}

	return true;
}

/* Miller-Rabin algorithm
 * - n: number to be tested
 * - r: random number in [2, n-2]
 * - m, k: n = m * 2^k, m is odd
 * - t: temporary buffer with 4*(msbn+2) size
 * - norm, msbnorm, sh: normalized n and shift bits
 */
static bool isprime_miller_rabin_1(
		const uint *n, int msbn, const uint *norm, int msbnorm, int sh,
		uint *r, int msbr, const uint *m, int msbm, int k, uint *t)
{
	uint tmp;
	int tmpi, msbx, msbt;
	const int tsz = (msbn + 2) * 2 * 4;
	const int xsz = (msbn + 2) * 4;
	const int qsz = xsz, rsz = xsz;
	uint *x = t + tsz / 4;
	uint *q = x + xsz / 4;

	/* x = 1 */
	msbx = 1;
	x[0] = 1;

	/* x = r^m % n */
	for (int i = 0; i < msbm; i++) {
		tmp = m[i];

		int bits = 31;
		if (i == msbm-1)
			bits = 32 - __builtin_clz(tmp);

		while (bits--) {
			if ((tmp & 0x1)) {
				/* x = (x * r) % n */
				memset(t, 0, tsz);
				msbt = _tt_int_mul_buf(t, x, msbx, r, msbr);
				if (_tt_int_cmp_buf(t, msbt, n, msbn) < 0) {
					msbx = msbt;
					memcpy(x, t, msbx*4);
					memset(x+msbx, 0, xsz-msbx*4);
				} else {
					memset(q, 0, qsz);
					memset(x, 0, xsz);
					msbt = _tt_int_shift_buf(t, msbt, sh);
					_tt_int_div_buf(q, &tmpi, x, &msbx,
							t, msbt, norm, msbnorm);
					msbx = _tt_int_shift_buf(x, msbx, -sh);
				}
			}

			/* r = r^2 % n */
			memset(t, 0, tsz);
			msbt = _tt_int_mul_buf(t, r, msbr, r, msbr);
			if (_tt_int_cmp_buf(t, msbt, n, msbn) < 0) {
				msbr = msbt;
				memcpy(r, t, msbr*4);
				memset(r+msbr, 0, rsz-msbr*4);
			} else {
				memset(q, 0, qsz);
				memset(r, 0, rsz);
				msbt = _tt_int_shift_buf(t, msbt, sh);
				_tt_int_div_buf(q, &tmpi, r, &msbr,
						t, msbt, norm, msbnorm);
				msbr = _tt_int_shift_buf(r, msbr, -sh);
			}

			tmp >>= 1;
		}
	}

	/* reuse r to store n-1 */
	const uint one = 1;
	memcpy(r, n, msbn*4);
	memset(r+msbn, 0, rsz-msbn*4);
	msbr = _tt_int_sub_buf(r, msbn, &one, 1);

	/* (x == 1 || x == n-1) -> prime */
	if ((msbx == 1 && x[0] == 1) ||
			(msbx == msbr && memcmp(x, r, msbx*4) == 0))
		return true;

	while (--k) {
		/* x = x^2 % n */
		memset(t, 0, tsz);
		msbt = _tt_int_mul_buf(t, x, msbx, x, msbx);
		if (_tt_int_cmp_buf(t, msbt, n, msbn) < 0) {
			msbx = msbt;
			memcpy(x, t, msbx*4);
			memset(x+msbx, 0, xsz-msbx*4);
		} else {
			memset(q, 0, qsz);
			memset(x, 0, xsz);
			msbt = _tt_int_shift_buf(t, msbt, sh);
			_tt_int_div_buf(q, &tmpi, x, &msbx,
					t, msbt, norm, msbnorm);
			msbx = _tt_int_shift_buf(x, msbx, -sh);
		}

		if (msbx == 1 && x[0] == 1)
			return false;
		if (msbx == msbr && memcmp(x, r, msbx*4) == 0)
			return true;
	}

	return false;
}

/* Pick random number in [2, n-2] */
static int pick_rand(const uint *n, int msbn, uint *r)
{
	int msbr;

	/* r -> [0, 2n-1] */
	for (int i = 0; i < msbn; i++)
		r[i] = _tt_rand() & ~BIT(31);
	const uint mask = BIT(32-__builtin_clz(n[msbn-1])) - 1;
	r[msbn-1] &= mask;
	msbr = _tt_int_get_msb(r, msbn);

	/* r -> [0, n-1] */
	if (_tt_int_cmp_buf(r, msbr, n, msbn) >= 0)
		msbr = _tt_int_sub_buf(r, msbr, n, msbn);

	/* r -> [3, n-1] */
	while (msbr == 1 && r[0] < 3)
		r[0] = _tt_rand() & ~BIT(31);

	/* r -> [2, n-2] */
	const uint one = 1;
	msbr = _tt_int_sub_buf(r, msbr, &one, 1);

	return msbr;
}

static bool isprime_miller_rabin(const uint *n, int msbn, int rounds)
{
	const int shift = __builtin_clz(n[msbn-1]) - 1;

	uint *buf;
	if (shift)
		buf = malloc((msbn+2)*7*4);
	else
		buf = malloc((msbn+2)*6*4);

	uint *m = buf;			/* size = msbn+2 */
	uint *r = m + msbn + 2;		/* size = msbn+2 */
	uint *t = r + msbn + 2;		/* size = (msbn+2)*4 */

	/* Normalize n for division */
	uint *norm = (uint *)n;
	int msbnorm = msbn;
	if (shift) {
		norm = t + (msbn+2)*4;
		memcpy(norm, n, msbn*4);
		memset(norm+msbn, 0, 8);
		msbnorm = _tt_int_shift_buf(norm, msbn, shift);
	}

	/* n-1 = m * 2^k */
	int k = 0, msbm = msbn;
	memcpy(m, n, msbn*4);
	m[0]--;

	/* Shift by word */
	while (m[0] == 0) {
		for (int i = 0; i < msbm-1; i++)
			m[i] = m[i+1];
		msbm--;
		k += 31;
	}

	/* Shift by bit */
	const int rsh = __builtin_ctz(m[0]);
	msbm = _tt_int_shift_buf(m, msbm, -rsh);
	k += rsh;

	bool ret = true;
	while (rounds--) {
		int msbr = pick_rand(n, msbn, r);
		if (!isprime_miller_rabin_1(n, msbn, norm, msbnorm, shift,
					r, msbr, m, msbm, k, t)) {
			ret = false;
			break;
		}
	}

	free(buf);
	return ret;
}

bool _tt_int_isprime_buf(const uint *ui, int msb, int rounds)
{
	if (msb == 1)
		return isprime_naive(ui[0]);
	if ((ui[0] & 0x1) == 0)
		return false;
	return isprime_miller_rabin(ui, msb, rounds);
}

bool tt_int_isprime(const struct tt_int *ti)
{
	/* 27 test rounds reaches 1/2^80 error bound */
	return _tt_int_isprime_buf(ti->_int, ti->_msb, 27);
}
