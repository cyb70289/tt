/* Primality testing
 *
 * Copyright (C) 2015 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/apn/integer.h>
#include <common/lib.h>
#include "integer.h"

#include <string.h>

#include "prime-tbl.c"

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

/* Test if ui is divisable by a word
 * - prime cannot be 2
 * - inverse = 1/prime mod beta (beta = 2^31 or 2^63)
 * - From "Modern Computer Arithmetic"
 */
static bool divisable(const _tt_word *ui, int msb, uint prime, _tt_word inverse)
{
	_tt_word x, q;
	uint b = 0;

	for (int i = 0; i < msb; i++) {
		if (b <= ui[i]) {
			x = ui[i] - b;
			b = 0;
		} else {
			x = ~(b - ui[i]) + 1;
			x &= ~_tt_word_top_bit;
			b = 1;
		}

		q = inverse * x;
		q &= ~_tt_word_top_bit;

		b += ((_tt_word_double)q * prime - x) >> _tt_word_bits;
	}
	return b == 0;
}

/* Test primality by mod small primes */
static bool maybe_prime(const _tt_word *ui, int msb)
{
	for (int i = 1; i < PRIMES_COUNT; i++) {
		if (divisable(ui, msb, _primes[i], _inverse[i]))
			return false;
	}
	return true;
}

/* Miller-Rabin test rounds to achieve 2^-80 error bound
 * - From "Handbook of Applied Cryptography"
 */
static int ml_rounds(int bits)
{
	static const struct {
		int bits;
		int rounds;
	} b2r[] = {
		{ 1300, 2 }, { 850, 3 }, { 650, 4 }, { 550, 5 }, { 450, 6 },
		{ 400, 7 }, { 350, 8 }, { 300, 9 }, { 250, 12 }, { 200, 15 },
		{ 150, 18 }, { 0, 27 },
	};

	for (int i = 0; i < ARRAY_SIZE(b2r); i++) {
		if (bits >= b2r[i].bits)
			return b2r[i].rounds;
	}
	return b2r[0].rounds;
}

/* Miller-Rabin algorithm
 * - n, msbn: number to be tested
 * - r, msbr: random number in [2, n-2] in montgomery form -> r*lambda % n
 *            buffer size must >= (msb+2) words
 * - m, msbm, k: n = m * 2^k, m is odd
 * - u: -1/n % 2^31
 * - w, msbw: lambda % n
 * - t: temporary buffer with 6*(msbn+2) words
 */
static bool isprime_miller_rabin_1(const _tt_word *n, int msbn,
		_tt_word *r, int msbr, const _tt_word *m, int msbm, int k,
		const _tt_word *u, int msbu, const _tt_word *w, int msbw,
		_tt_word *t)
{
	int msbx, msbt;
	const int tsz = (msbn + 2) * 2 * _tt_word_sz;
	const int xsz = (msbn + 2) * _tt_word_sz;
	_tt_word *x = t + tsz / _tt_word_sz;
	_tt_word *buf = x + xsz / _tt_word_sz;	/* (msbn+2)*3 words */
	const int rsz = xsz;

	/* x = r */
	msbx = msbr;
	memcpy(x, r, msbr*_tt_word_sz);

	/* x = r^m % n */
	for (int i = msbm-1; i >= 0; i--) {
		_tt_word tmp = m[i];

		int bits = _tt_word_bits;
		if (i == msbm-1) {
			bits = _tt_int_word_bits(tmp) - 1;
			tmp <<= (_tt_word_bits - bits);
		}

		while (bits--) {
			/* x = x^2 % n */
			memset(t, 0, tsz);
			msbt = _tt_int_mul_buf(t, x, msbx, x, msbx);
			_tt_int_mont_reduce(t, &msbx, t, msbt, u, msbu,
					n, msbn, buf);
			memcpy(x, t, msbx*_tt_word_sz);

			if (tmp & (_tt_word_top_bit >> 1)) {
				/* x = (x * r) % n */
				memset(t, 0, tsz);
				msbt = _tt_int_mul_buf(t, x, msbx, r, msbr);
				_tt_int_mont_reduce(t, &msbx, t, msbt, u, msbu,
						n, msbn, buf);
				memcpy(x, t, msbx*_tt_word_sz);
			}
			memset(x+msbx, 0, xsz-msbx*_tt_word_sz);

			tmp <<= 1;
		}
	}

	/* r = n-1 -> n-lambda%n */
	memcpy(r, n, msbn*_tt_word_sz);
	memset(r+msbn, 0, rsz-msbn*_tt_word_sz);
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
		memcpy(x, t, msbx*_tt_word_sz);
		memset(x+msbx, 0, xsz-msbx*_tt_word_sz);

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
static int pick_rand(_tt_word *r, const _tt_word *n, int msbn)
{
	int msbr;

	/* r -> [0, 2n-1] */
	for (int i = 0; i < msbn; i++) {
		r[i] = _tt_rand() & ~BIT(31);
#ifdef _TT_LP64_
		r[i] <<= 32;
		r[i] |= _tt_rand();
#endif
	}
	const _tt_word mask = (1ULL << _tt_int_word_bits(n[msbn-1])) - 1;
	r[msbn-1] &= mask;
	msbr = _tt_int_get_msb(r, msbn);

	/* r -> [0, n-1] */
	if (_tt_int_cmp_buf(r, msbr, n, msbn) >= 0)
		msbr = _tt_int_sub_buf(r, msbr, n, msbn);

	/* r -> [3, n-1] */
	while (msbr == 1 && r[0] < 3)
		r[0] = _tt_rand() & ~BIT(31);

	/* r -> [2, n-2] */
	const _tt_word one = 1;
	msbr = _tt_int_sub_buf(r, msbr, &one, 1);

	return msbr;
}

static bool isprime_miller_rabin(const _tt_word *n, int msbn, int rounds)
{
	_tt_word *buf;
	buf = malloc((msbn+2)*7*_tt_word_sz);

	_tt_word *m = buf;		/* (msbn+2) words */
	_tt_word *t = m + (msbn+2);	/* (msbn+2)*6 words */

	struct tt_int ni = _TT_INT_DECL(msbn, n);

	/* lambda = (2^31)^msbn, (2^63)^msbn */
	struct tt_int *lambda = tt_int_alloc();
	_tt_int_realloc(lambda, msbn+1);
	lambda->buf[msbn] = 1;
	lambda->msb = msbn + 1;

	/* u = -1/n % lambda, u >= 0 */
	struct tt_int *u = tt_int_alloc();
	tt_int_mod_inv(u, &ni, lambda);
	if (u->sign == 0)
		tt_int_sub(u, lambda, u);
	/* w = lambda % n */
	struct tt_int *w = tt_int_alloc();
	tt_int_div(NULL, w, lambda, &ni);

	/* n-1 = m * 2^k */
	int k = 0, msbm = msbn;
	memcpy(m, n, msbn*_tt_word_sz);
	m[0]--;
	/* Shift by word */
	while (m[0] == 0) {
		for (int i = 0; i < msbm-1; i++)
			m[i] = m[i+1];
		msbm--;
		k += _tt_word_bits;
	}
	/* Shift by bit */
	const int rsh = _tt_int_word_ctz(m[0]);
	msbm = _tt_int_shift_buf(m, msbm, -rsh);
	k += rsh;

	bool ret = true;
	struct tt_int *r = tt_int_alloc();
	_tt_int_realloc(r, msbn+2);
	while (rounds--) {
		r->msb = pick_rand(r->buf, n, msbn);
		/* r -> r*lambda % n */
		memset(t, 0, (msbn+2)*2*_tt_word_sz);
		int msbt = _tt_int_mul_buf(t, r->buf, r->msb,
				lambda->buf, lambda->msb);
		struct tt_int ti = _TT_INT_DECL(msbt, t);
		tt_int_div(NULL, r, &ti, &ni);

		if (!isprime_miller_rabin_1(n, msbn, r->buf, r->msb, m, msbm,
				k, u->buf, u->msb, w->buf, w->msb, t)) {
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

bool _tt_int_isprime_buf(const _tt_word *ui, int msb)
{
	if (msb == 1 && ui[0] < BIT(31))
		return isprime_naive(ui[0]);

	if ((ui[0] & 0x1) == 0)
		return false;

	if (!maybe_prime(ui, msb))
		return false;

	int bits = (msb - 1) * _tt_word_bits;
	bits += _tt_int_word_bits(ui[msb-1]);
	return isprime_miller_rabin(ui, msb, ml_rounds(bits));
}

bool tt_int_isprime(const struct tt_int *ti)
{
	return _tt_int_isprime_buf(ti->buf, ti->msb);
}
