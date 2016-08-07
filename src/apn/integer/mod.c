/* Modular
 *
 * Copyright (C) 2016 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/apn/integer.h>
#include <common/lib.h>
#include "integer.h"

#include <string.h>

/* Modular inverse
 * - m = 1/a mod b
 * - a, b must be coprime
 */
int tt_int_mod_inv(struct tt_int *m,
		const struct tt_int *a, const struct tt_int *b)
{
	struct tt_int *n = tt_int_alloc();

	int ret = tt_int_extgcd(NULL, m, n, a, b);
	tt_int_free(n);

	return ret;
}

/* Montgomery reduce (beta = 2^31 or 2^63, lambda = beta^msbn)
 * - r: result, may share buffer with c, size >= (msbn+1) words
 * - c: to be reduced
 * - u: -1/n % lambda
 * - n: modulus
 * - t: temp buffer, size >= 3*(msbn+1) words
 */
int _tt_int_mont_reduce(_tt_word *r, int *msbr, const _tt_word *c, int msbc,
		const _tt_word *u, int msbu, const _tt_word *n, int msbn,
		_tt_word *t)
{
	tt_assert_fa(msbc <= msbn*2);

	/* q = u*c % lambda (TODO: drop 1/4 calculation) */
	_tt_word *q = t;
	memset(q, 0, msbn*2*_tt_word_sz);
	int msbq = _tt_int_mul_buf(q, u, msbu, c, _tt_min(msbc, msbn));
	if (msbq > msbn)
		msbq = _tt_int_get_msb(q, msbn);
	/* Q = c + q*n */
	_tt_word *Q = q + msbq;
	memset(Q, 0, msbn*2*_tt_word_sz);
	int msbQ = _tt_int_mul_buf(Q, q, msbq, n, msbn);
	msbQ = _tt_int_add_buf(Q, msbQ, c, msbc);
	/* r = Q / lambda */
	memset(r, 0, (msbn+1)*_tt_word_sz);
	*msbr = 1;
	if (msbQ > msbn) {
		*msbr = msbQ - msbn;
		memcpy(r, Q+msbn, (*msbr)*_tt_word_sz);
	}
	if (_tt_int_cmp_buf(r, *msbr, n, msbn) >= 0)
		*msbr = _tt_int_sub_buf(r, *msbr, n, msbn);

	tt_assert_fa(_tt_int_cmp_buf(r, *msbr, n, msbn) < 0);

	return 0;
}
