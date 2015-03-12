/* Rounding
 *
 * Copyright (C) 2014 Yibo Cai
 */
#pragma once

/* Rounding methods */
enum {
	TT_ROUND_HALF_EVEN = 1,	/* 11.5 -> 12, 12.5 -> 12 */
	TT_ROUND_HALF_AWAY0,	/* 23.5 -> 24, -23.5 -> -24 */
	TT_ROUND_DOWN,		/* Truncate */

	TT_ROUND_MAX,
};

int tt_set_rounding(uint rnd);
