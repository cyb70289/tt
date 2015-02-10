/* types.h
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
