/* Rounding
 *
 * Copyright (C) 2015 Yibo Cai
 */
#include <tt/tt.h>

/* Default: round half to even */
static int _rounding = TT_ROUND_HALF_EVEN;

int tt_set_rounding(uint rnd)
{
	tt_assert(rnd < TT_ROUND_MAX);

	int r = _rounding;

	if (rnd == 0)
		_rounding = TT_ROUND_HALF_EVEN;
	else
		_rounding = rnd;

	return r;
}

/* Rounding
 * - odd: last digit is odd?
 * - dig: digit to be rounded, 0 ~ 9
 * - rnd: rounding method, 0 - use current method
 * - return: 0 - drop, 1 - increment abs
 */
int tt_round(int odd, uint dig, uint rnd)
{
	tt_assert(rnd < TT_ROUND_MAX);

	if (dig < 5)
		return 0;
	else if (dig > 5)
		return 1;

	if (rnd == 0)
		rnd = _rounding;

	switch (rnd) {
	case TT_ROUND_HALF_AWAY0:
		return 1;
	case TT_ROUND_HALF_EVEN:
		return !!odd;
	default:
		return 0;
	}
}
