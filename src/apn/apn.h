/* Arbitrary precision number
 *
 * Copyright (C) 2015 Yibo Cai
 *
 * References:
 * - General Decimal Arithmetic Specification. Michael F. Cowlishaw.
 * - Decimal Floating-Point: Algorism for Computers. Michael F. Cowlishaw.
 */
#pragma once

/* Arbitrary precision decimal: { sign, dig, exp } = (-1)^sign * dig * 10^exp
 * +3.14  -> *dig = 314,  exp = -2, sign = 0, msb = 3
 * +0.220 -> *dig = 220,  exp = -3, sign = 0, msb = 3
 * -0.003 -> *dig = 3,    exp = -3, sign = 1, msb = 1
 * 123400 -> *dig = 1234, exp = 2,  sign = 0, msb = 4
 * 0      -> *dig = 0,    exp = 0,  sign = 0, msb = 1
 * 0.00   -> *dig = 0,    exp = -2, sign = 0, msb = 1
 */
struct tt_apn {
	short _sign;		/* Sign: 0, 1 */
	short _inf_nan;		/* 1 - Inf, 2 - NaN */
#define TT_APN_INF	1
#define TT_APN_NAN	2

	int _exp;		/* Decimal exponent */
	const int _prec;	/* Precision: maximum significant digits */
	const int _prec_full;	/* Full precision digits used internally */
#define TT_APN_PREC_RND	9	/* Extra precison digits for rounding guard */
#define TT_APN_PREC_CRY	1	/* Carry digit */
#define TT_APN_PREC_ALN	8	/* Append to align-9 shifting */
	const uint _digsz;	/* Bytes of _dig[] */
	int _msb;		/* Current decimal digit count
				 * - must be <= _prec after calculation
				 * - may exceed _prec by EXT_PREC in calculation
				 */
	uint *_dig32;		/* Significand buffer
				 * Bit -> 31  29                        0
				 *        +--+--------------------------+
				 * Val -> |00| Decimal: 0 ~ 999,999,999 |
				 *        +--+--------------------------+
				 * - Each uint contains 9-digit decimals
				 */
};

/* Clear to zero */
void _tt_apn_zero(struct tt_apn *apn);

/* Check if significand == 0, it's not a true zero if _exp < 0 */
static inline bool _tt_apn_is_zero(const struct tt_apn *apn)
{
	return apn->_msb == 1 && *apn->_dig32 == 0 && !apn->_inf_nan;
}

static inline bool _tt_apn_is_true_zero(const struct tt_apn *apn)
{
	return _tt_apn_is_zero(apn) && apn->_exp == 0;
}

/* Check sanity */
int _tt_apn_sanity(const struct tt_apn *apn);

/* Get digits in a uint */
static inline void _tt_apn_to_d9(uint dig, uchar *d)
{
	for (int i = 0; i < 9; i++, dig /= 10)
		d[i] = dig % 10;
}

/* Get "pos-th" digit (pos starts from 0) */
uint _tt_apn_get_dig(const uint *dig, int pos);

/* uint64/128 -> decimal */
int _tt_apn_uint_to_dec(uint *dig32, uint64_t num);
int _tt_apn_uint128_to_dec(uint *dig32, __uint128_t num);
