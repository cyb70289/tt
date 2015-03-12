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

/* Convert num(>0) to [10^18, 10^19), return adjusted exponent.
 * Based on uClibc-0.9.33 source: libc/stdio/_fpmaxtostr.c
 */
static int norm_fp_to_i64(double num, uint64_t *ui64)
{
	static const long double _exp10[] = {
		1E1L, 1E2L, 1E4L, 1E8L, 1E16L, 1E32L, 1E64L, 1E128L, 1E256L
	};
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

	/* Convert to [10^18, 10^19). 19 significands are more than enough for
	 * a double float number, whose precision is less than 17 decimals. */
	uint64_t ui64;
	int adjexp = norm_fp_to_i64(num, &ui64);
	tt_apn_from_uint(apn, ui64);
	apn->_exp += adjexp;
	apn->_sign = sign;

	return 0;
}
