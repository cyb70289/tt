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
 * - n, msbn: number to be tested
 * - r, msbr: random number in [2, n-2] in montgomery form -> r*lambda % n
 *            buffer size must >= (msb+2) uints
 * - m, msbm, k: n = m * 2^k, m is odd
 * - u: -1/n % 2^31
 * - w, msbw: lambda % n
 * - t: temporary buffer with 6*(msbn+2) uints
 */
static bool isprime_miller_rabin_1(const uint *n, int msbn, uint *r, int msbr,
		const uint *m, int msbm, int k, const uint *u, int msbu,
		const uint *w, int msbw, uint *t)
{
	int msbx, msbt;
	const int tsz = (msbn + 2) * 2 * 4;
	const int xsz = (msbn + 2) * 4;
	uint *x = t + tsz / 4;
	uint *buf = x + xsz / 4;	/* (msbn+2)*3 uints */
	const int rsz = xsz;

	/* x = 1 -> lambda % n */
	msbx = msbw;
	memcpy(x, w, msbx*4);

	/* x = r^m % n */
	for (int i = 0; i < msbm; i++) {
		uint tmp = m[i];

		int bits = 31;
		if (i == msbm-1)
			bits = 32 - __builtin_clz(tmp);

		while (bits--) {
			if ((tmp & 0x1)) {
				/* x = (x * r) % n */
				memset(t, 0, tsz);
				msbt = _tt_int_mul_buf(t, x, msbx, r, msbr);
				_tt_int_mont_reduce(t, &msbx, t, msbt, u, msbu,
						n, msbn, buf);
				memcpy(x, t, msbx*4);
				memset(x+msbx, 0, xsz-msbx*4);
			}

			/* r = r^2 % n */
			memset(t, 0, tsz);
			msbt = _tt_int_mul_buf(t, r, msbr, r, msbr);
			_tt_int_mont_reduce(t, &msbr, t, msbt, u, msbu,
					n, msbn, buf);
			memcpy(r, t, msbr*4);
			memset(r+msbr, 0, rsz-msbr*4);

			tmp >>= 1;
		}
	}

	/* r = n-1 -> n-lambda%n */
	memcpy(r, n, msbn*4);
	memset(r+msbn, 0, rsz-msbn*4);
	msbr = _tt_int_sub_buf(r, msbn, w, msbw);

	/* prime: x == 1 -> x == lambda%n */
	if (_tt_int_cmp_buf(x, msbx, w, msbw) == 0)
		return true;
	/* prime: x == n-1 -> x == n-lambda%n */
	if (_tt_int_cmp_buf(x, msbx, r, msbr) == 0)
		return true;

	while (--k) {
		/* x = x^2 % n */
		memset(t, 0, tsz);
		msbt = _tt_int_mul_buf(t, x, msbx, x, msbx);
		_tt_int_mont_reduce(t, &msbx, t, msbt, u, msbu, n, msbn, buf);
		memcpy(x, t, msbx*4);
		memset(x+msbx, 0, xsz-msbx*4);

		/* composite: x == 1 -> x == lambda%n */
		if (_tt_int_cmp_buf(x, msbx, w, msbw) == 0)
			return false;
		/* prime: x == n-1 -> x == n-lambda%n */
		if (_tt_int_cmp_buf(x, msbx, r, msbr) == 0)
			return true;
	}

	/* composite */
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
	uint *buf;
	buf = malloc((msbn+2)*7*4);

	uint *m = buf;		/* (msbn+2) uints*/
	uint *t = m + (msbn+2);	/* (msbn+2)*6 uints */

	struct tt_int ni = _TT_INT_DECL(msbn, n);

	/* lambda = (2^31)^msbn */
	struct tt_int *lambda = tt_int_alloc();
	_tt_int_realloc(lambda, msbn+1);
	lambda->_int[msbn] = 1;
	lambda->_msb = msbn+1;

	/* u = -1/n % lambda, u >= 0 */
	struct tt_int *u = tt_int_alloc();
	tt_int_mod_inv(u, &ni, lambda);
	if (u->_sign == 0)
		tt_int_sub(u, lambda, u);
	/* w = lambda % n */
	struct tt_int *w = tt_int_alloc();
	tt_int_div(NULL, w, lambda, &ni);

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
	struct tt_int *r = tt_int_alloc();
	_tt_int_realloc(r, msbn+2);
	while (rounds--) {
		r->_msb = pick_rand(n, msbn, r->_int);
		/* r -> r*lambda % n */
		memset(t, 0, (msbn+2)*2*4);
		int msbt = _tt_int_mul_buf(t, r->_int, r->_msb,
				lambda->_int, lambda->_msb);
		struct tt_int ti = _TT_INT_DECL(msbt, t);
		tt_int_div(NULL, r, &ti, &ni);

		if (!isprime_miller_rabin_1(n, msbn, r->_int, r->_msb, m, msbm,
				k, u->_int, u->_msb, w->_int, w->_msb, t)) {
			ret = false;
			break;
		}
		_tt_int_zero(r);
	}

	free(buf);
	tt_int_free(lambda);
	tt_int_free(u);
	tt_int_free(w);
	tt_int_free(r);
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
