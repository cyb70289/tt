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
	const uint _prec;	/* Precision: maximum significant digits */
#define TT_APN_EXT_PREC	9	/* Extra precison digits for rounding guard */

	const uint _digsz;	/* Bytes of _dig[] */
	uint _msb;		/* Current decimal digit count
				 * - must be <= _prec after calculation
				 * - may exceed _prec by EXT_PREC in calculation
				 */
	uint *_dig32;		/* Significand buffer
				 * Bit -> 31      22 21 20     11 10 9       0
				 *        +--------+---+--------+---+--------+
				 * Val -> | digit3 | G | digit2 | G | digit1 |
				 *        +--------+---+--------+---+--------+
				 * - Each uint contains three 3-digit decimals
				 * - Bit-10,21 guard add/sub overflow
				 * - Stored in little endian
				 */
};

struct tt_apn *tt_apn_alloc(uint prec);
void tt_apn_free(struct tt_apn *apn);

/* Conversion */
int tt_apn_from_string(struct tt_apn *apn, const char *str);
int tt_apn_from_sint(struct tt_apn *apn, int64_t num);
int tt_apn_from_uint(struct tt_apn *apn, uint64_t num);
int tt_apn_from_float(struct tt_apn *apn, double num);
int tt_apn_to_string(const struct tt_apn *apn, char *str, uint len);
