/* Arbitrary precision integer
 *
 * Copyright (C) 2015 Yibo Cai
 */
#pragma once

struct tt_int {
	int _sign;		/* Sign: 0/+, 1/- */

#define TT_INT_MAX	19	/* 2^(19*31)-1 --> 177 full decimal digits */
#define TT_INT_GUARD	1	/* Guard ints at top of _int[] */
	const uint __sz;	/* Size of _int[] buffer in ints */
	const uint _max;	/* Maximum integers in _int[], 1 ~ __sz-1 */
	uint _msb;		/* Valid integers in _int[], 1 ~ _max */

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

/* Clear zero */
void _tt_int_zero(struct tt_int *ti);

static inline bool _tt_int_is_zero(const struct tt_int *ti)
{
	return ti->_msb == 1 && ti->_int[0] == 0;
}

int _tt_int_sanity(const struct tt_int *ti);
