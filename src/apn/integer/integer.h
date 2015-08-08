/* Arbitrary precision integer
 *
 * Copyright (C) 2015 Yibo Cai
 */
#pragma once

struct tt_int {
	int _sign;		/* Sign: 0/+, 1/- */

#define TT_INT_CNT	16	/* 2^(16*31)-1 --> 150 decimal digits */
	uint _cnt;		/* Count of integers in _int[] */
	uint _msb;		/* Valid integers in _int[], must > 0 */

	uint *_int;		/* Integer array
				 * Bit -> 31 30                     0
				 *        +-+-----------------------+
				 * Val -> |0| Integer: 0 ~ 2^31 - 1 |
				 *        +-+-----------------------+
				 * - Each uint contains 31 bits
				 */
};

/* Clear zero */
void _tt_int_zero(struct tt_int *ti);

static inline bool _tt_int_is_zero(const struct tt_int *ti)
{
	return ti->_msb == 1 && ti->_int[0] == 0;
}

int _tt_int_sanity(const struct tt_int *ti);
