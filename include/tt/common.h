/* common.h
 *
 * Copyright (C) 2014 Yibo Cai
 */
#pragma once

#ifndef HAVE_CONFIG_H
#error "Include tt.h first!"
#endif

#if CONFIG_DOUBLE
typedef double	tt_float;
#else
typedef float	tt_float;
#endif

typedef uint8_t uchar;
typedef uint16_t ushort;
typedef uint32_t uint;

/* Rounding */
enum {
	TT_ROUND_HALF_EVEN = 1,	/* 11.5 -> 12, 12.5 -> 12 */
	TT_ROUND_HALF_AWAY0,	/* 23.5 -> 24, -23.5 -> -24 */
	TT_ROUND_DOWN,		/* Truncate */

	TT_ROUND_MAX,
};

int tt_set_rounding(uint rnd);
int tt_round(int odd, uint dig, uint rnd);

/* Key */
struct tt_key {
	int size;	/* Element size in bytes */
	int (*cmp)(const struct tt_key *num,
			const void *v1, const void *v2);/* Compare */
	void (*swap)(const struct tt_key *num,
			void *v1, void *v2);		/* Swap */

	void (*_set)(const struct tt_key *num,
			void *dst, const void *src);	/* Set value */
};
