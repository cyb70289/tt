/* Machine number --> APN
 *
 * Copyright (C) 2015 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/apn/apn.h>
#include <tt/common/round.h>
#include <common/lib.h>
#include "apn.h"

#include <string.h>
#include <math.h>

static const long double _exp10[] = {
	1E1L, 1E2L, 1E4L, 1E8L, 1E16L, 1E32L, 1E64L, 1E128L, 1E256L
};

/* Convert num(>0) to [10^18, 10^19), return adjusted exponent.
 * Based on uClibc-0.9.33 source: libc/stdio/_fpmaxtostr.c
 */
static int norm_fp_to_i64(double num, uint64_t *ui64)
{
	static const long double _lower = 1E18L, _upper = 1E19L;

	long double lnum = num;
	int adjexp = 0, i = ARRAY_SIZE(_exp10);
	if (lnum < _lower) {
		/* Scale up */
		while (i--) {
			if (lnum * _exp10[i] < _upper) {
				lnum *= _exp10[i];
				adjexp -= (1U << i);
			}
		}
	} else {
		/* Scale down */
		while (i--) {
			if (lnum / _exp10[i] >= _lower) {
				lnum /= _exp10[i];
				adjexp += (1U << i);
			}
		}
	}
	if (lnum >= _upper) {
		lnum /= _exp10[0];
		adjexp++;
	}
	tt_assert(lnum >= _lower && lnum < _upper);

	*ui64 = (uint64_t)lnum;
	*ui64 += _tt_round((*ui64) & 1, (uint)((lnum - *ui64) * 10),
			TT_ROUND_HALF_AWAY0);

	return adjexp;
}

int tt_apn_from_uint(struct tt_apn *apn, uint64_t num)
{
	tt_assert(apn->_prec >= 20);

	_tt_apn_zero(apn);
	if (num)
		apn->_msb = 0;	/* Compensate "0" */

	int ptr = 0;
	uint *dig32 = apn->_dig32;
	while (num) {
		/* Get 3 decimals */
		uint dec3 = num % 1000;
		num /= 1000;

		/* Increment significand */
		if (num || dec3 > 99)
			apn->_msb += 3;
		else if (dec3 > 9)
			apn->_msb +=2;
		else
			apn->_msb++;

		/* Fill digit buffer */
		*dig32 |= (dec3 << ptr);
		ptr += 11;
		if (ptr > 32) {
			ptr = 0;
			dig32++;
		}
	}

	return 0;
}

int tt_apn_from_sint(struct tt_apn *apn, int64_t num)
{
	short sign;
	uint64_t unum;

	if (num < 0) {
		sign = 1;
		unum = -num;
	} else {
		sign = 0;
		unum = num;
	}

	int ret = tt_apn_from_uint(apn, unum);
	apn->_sign = sign;

	return ret;
}

int tt_apn_from_float(struct tt_apn *apn, double num)
{
	tt_assert(apn->_prec >= 20);

	_tt_apn_zero(apn);

	int sign = !!signbit(num);
	switch (fpclassify(num)) {
	case FP_NAN:
		apn->_inf_nan = TT_APN_NAN;
		return 0;
	case FP_INFINITE:
		apn->_inf_nan = TT_APN_INF;
		return 0;
	case FP_ZERO:
		return 0;
	default:
		break;
	}

	if (sign)
		num = -num;

	/* Convert to [10^18, 10^19). 19 significands is enough to keep
	 * precision of a double float number.
	 */
	uint64_t ui64;
	int adjexp = norm_fp_to_i64(num, &ui64);
	tt_apn_from_uint(apn, ui64);
	apn->_exp = adjexp;
	apn->_sign = sign;

	return 0;
}

#ifdef __STDC_IEC_559__
/* IEEE-754 double precision number representation
 *
 * Layout(little endian):
 * +-------+----------------+
 * |  Bit  | Representation |
 * +-------+----------------+
 * |  63   |      Sign      |
 * +-------+----------------+
 * | 62~52 |    Exponent    |
 * +-------+----------------+
 * | 51~0  |    Mantissa    |
 * +-------+----------------+
 *
 * Value:
 * +---+--------+---------+---------------------------+
 * | S |    E   |    M    |           Value           |
 * +---+--------+---------+---------------------------+
 * | - | 1~2046 |    -    | (-1)^S * 2^(E-1023) * 1.M |
 * +---+--------+---------+---------------------------+
 * | - |   0    | nonzero | (-1)^S * 2^(-1022) * 0.M  |
 * +---+--------+---------+---------------------------+
 * | 0 |   0    |    0    |            +0.0           |
 * +---+--------+---------+---------------------------+
 * | 1 |   0    |    0    |            -0.0           |
 * +---+--------+---------+---------------------------+
 * | 0 |  2047  |    0    |            +Inf           |
 * +---+--------+---------+---------------------------+
 * | 1 |  2047  |    0    |            -Inf           |
 * +---+--------+---------+---------------------------+
 * | - |  2047  | nonzero |            NaN            |
 * +---+--------+---------+---------------------------+
 */
int tt_apn_to_float(const struct tt_apn *apn, double *num)
{
	int ret = 0;
	union {
		uint64_t i;
		double d;
	} v;
	struct tt_apn *apn_mm = NULL;

	/* Check zero */
	if (_tt_apn_is_zero(apn)) {
		v.i = 0;
		goto out;
	}

	/* Check NaN, Inf */
	if (apn->_inf_nan == TT_APN_INF) {
		v.i = 2047ULL << 52;    /* Inf */
		ret = TT_APN_EOVERFLOW;
		goto out;
	} else if (apn->_inf_nan == TT_APN_NAN) {
		v.i = (2047ULL << 52) | ((1ULL << 52) - 1);
		ret = TT_APN_EINVAL;
		goto out;
	}

	apn_mm = tt_apn_alloc(0);

	/* Check overflow */
	v.i = (2046ULL << 52) | ((1ULL << 52) - 1);	/* Max double */
	tt_apn_from_float(apn_mm, v.d);
	if (tt_apn_cmp_abs(apn, apn_mm) >= 0) {
		v.i = 2047ULL << 52;	/* Inf */
		ret = TT_APN_EOVERFLOW;
		tt_warn("Float overflow");
		goto out;
	}

	/* Check underflow
	 * XXX: Unnormalized double can handle 4.94E-324.
	 * But it causes underrun in the div loop.
	 * We restrict to normalized double bounded to 2.22E-308.
	 */
	v.i = 1ULL << 52;
	if (tt_apn_cmp_abs(apn_mm, apn) >= 0) {
		v.i = 0;
		ret = TT_APN_EUNDERFLOW;
		tt_warn("Float underflow");
		goto out;
	}

	/* Round to 19 significands */
	int sfr = 0, rnd = 0;
	if (apn->_msb > 19) {
		sfr = apn->_msb - 19;
		rnd = _tt_round(_tt_apn_get_dig(apn->_dig32, sfr) & 1,
					_tt_apn_get_dig(apn->_dig32, sfr-1),
					TT_ROUND_HALF_AWAY0);
	}

	/* Calculate significand */
	long double ld = 0;
	for (int i = apn->_msb - 1; i >= sfr; i--) {
		/* We can do much better here. Is it necessary? */
		ld *= 10;
		ld += _tt_apn_get_dig(apn->_dig32, i);
	}
	ld += rnd;	/* May cause overflow? */

	/* Multiply/divide exponent */
	int _exp = apn->_exp + sfr;
	int i = ARRAY_SIZE(_exp10) - 1;	/* _exp = 10^(2^i) */
	if (_exp > 0) {
		while (_exp > 0) {
			if (_exp >= 1U << i) {
				ld *= _exp10[i];
				_exp -= 1U << i;
			}
			i--;
		}
	} else if (_exp < 0) {
		_exp = -_exp;
		while (_exp > 0) {
			if (_exp >= 1U << i) {
				ld /= _exp10[i];
				_exp -= 1U << i;
			}
			i--;
		}
	}
	v.d = ld;

out:
	if (apn_mm)
		tt_apn_free(apn_mm);

	/* Check sign */
	if (apn->_sign)
		v.i |= 1ULL << 63;

	*num = v.d;
	return ret;
}
#endif
