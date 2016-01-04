/* Arbitrary precision integer
 *
 * Copyright (C) 2015 Yibo Cai
 */
#pragma once

struct tt_int {
	int _sign;		/* Sign: 0/+, 1/- */

#define TT_INT_DEF	19	/* 2^(19*31)-1 --> 177 decimal digits */
#define TT_INT_GUARD	1	/* Guard ints at top of _int[] */
	const int __sz;		/* Size of _int[] buffer in ints */
	const int _max;		/* Maximum integers in _int[], 1 ~ __sz-1 */
	int _msb;		/* Valid integers in _int[], 1 ~ _max */

	uint *_int;		/* Integer array
				 * Bit -> 31 30                     0
				 *        +-+-----------------------+
				 * Val -> |0| Integer: 0 ~ 2^31 - 1 |
				 *        +-+-----------------------+
				 * - Each uint contains 31 bits
				 * - Top uint is reserved, cannot use
				 */
};

int _tt_int_realloc(struct tt_int *ti, uint msb);

int _tt_int_copy(struct tt_int *dst, const struct tt_int *src);

/* Clear zero */
void _tt_int_zero(struct tt_int *ti);

static inline bool _tt_int_is_zero(const struct tt_int *ti)
{
	return ti->_msb == 1 && ti->_int[0] == 0;
}

int _tt_int_sanity(const struct tt_int *ti);

int _tt_int_add_buf(uint *int1, int msb1, const uint *int2, int msb2);
int _tt_int_sub_buf(uint *int1, int msb1, const uint *int2, int msb2);
int _tt_int_mul_buf(uint *intr, const uint *int1, int msb1,
		const uint *int2, int msb2);
int _tt_int_div_buf(uint *qt, int *msb_qt, uint *rm, int *msb_rm,
		const uint *dd, int msb_dd, const uint *ds, int msb_ds);
int _tt_int_shift_buf(uint *_int, int msb, int shift);
int _tt_int_cmp_buf(const uint *int1, int msb1, const uint *int2, int msb2);
int _tt_int_get_msb(const uint *ui, int len);

int _tt_int_gcd_buf(uint *g, int *msb,
		const uint *a, int msba, const uint *b, int msbb);
bool _tt_int_isprime_buf(const uint *ui, int msb, int rounds);
