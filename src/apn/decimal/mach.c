/* Machine number --> APN
 *
 * Copyright (C) 2015 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/apn/decimal.h>
#include <tt/common/round.h>
#include <common/lib.h>
#include "decimal.h"

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

int tt_dec_from_uint(struct tt_dec *dec, uint64_t num)
{
	tt_assert(dec->_prec >= 20);

	_tt_dec_zero(dec);
	dec->_msb = _tt_dec_uint_to_dec(dec->_dig32, num);

	return 0;
}

int tt_dec_from_sint(struct tt_dec *dec, int64_t num)
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

	int ret = tt_dec_from_uint(dec, unum);
	dec->_sign = sign;

	return ret;
}

int tt_dec_from_float(struct tt_dec *dec, double num)
{
	tt_assert(dec->_prec >= 20);

	_tt_dec_zero(dec);

	int sign = !!signbit(num);
	switch (fpclassify(num)) {
	case FP_NAN:
		dec->_inf_nan = TT_DEC_NAN;
		return 0;
	case FP_INFINITE:
		dec->_inf_nan = TT_DEC_INF;
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
	tt_dec_from_uint(dec, ui64);
	dec->_exp = adjexp;
	dec->_sign = sign;

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
int tt_dec_to_float(const struct tt_dec *dec, double *num)
{
	int ret = 0;
	union {
		uint64_t i;
		double d;
	} v;
	struct tt_dec *dec_mm = NULL;

	/* Check zero */
	if (_tt_dec_is_zero(dec)) {
		v.i = 0;
		goto out;
	}

	/* Check NaN, Inf */
	if (dec->_inf_nan == TT_DEC_INF) {
		v.i = 2047ULL << 52;    /* Inf */
		ret = TT_APN_EOVERFLOW;
		goto out;
	} else if (dec->_inf_nan == TT_DEC_NAN) {
		v.i = (2047ULL << 52) | ((1ULL << 52) - 1);
		ret = TT_APN_EINVAL;
		goto out;
	}

	dec_mm = tt_dec_alloc(0);

	/* Check overflow */
	v.i = (2046ULL << 52) | ((1ULL << 52) - 1);	/* Max double */
	tt_dec_from_float(dec_mm, v.d);
	if (tt_dec_cmp_abs(dec, dec_mm) >= 0) {
		v.i = 2047ULL << 52;	/* Inf */
		ret = TT_APN_EOVERFLOW;
		tt_warn("Float overflow");
		goto out;
	}

	/* Check underflow
	 * XXX: Unnormalized double bound may cause underrun in the div loop.
	 */
#if 1
	v.i = 1;		/* Unnormalized, 4.94E-324 */
#else
	v.i = 1ULL << 52;	/* NOrmalized, 2.22E-308 */
#endif
	tt_dec_from_float(dec_mm, v.d);
	if (tt_dec_cmp_abs(dec_mm, dec) >= 0) {
		v.i = 0;
		ret = TT_APN_EUNDERFLOW;
		tt_warn("Float underflow");
		goto out;
	}

	/* Round to 19 significands */
	int sfr = 0, rnd = 0;
	if (dec->_msb > 19) {
		sfr = dec->_msb - 19;
		rnd = _tt_round(_tt_dec_get_dig(dec->_dig32, sfr) & 1,
					_tt_dec_get_dig(dec->_dig32, sfr-1),
					TT_ROUND_HALF_AWAY0);
	}

	/* Calculate significand */
	long double ld = 0;
	for (int i = dec->_msb - 1; i >= sfr; i--) {
		/* We can do much better here. Is it necessary? */
		ld *= 10;
		ld += _tt_dec_get_dig(dec->_dig32, i);
	}
	ld += rnd;	/* May cause overflow? */

	/* Multiply/divide exponent */
	int _exp = dec->_exp + sfr;
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
	if (dec_mm)
		tt_dec_free(dec_mm);

	/* Check sign */
	if (dec->_sign)
		v.i |= 1ULL << 63;

	*num = v.d;
	return ret;
}
#endif
