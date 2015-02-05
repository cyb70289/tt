/* Common libs
 *
 * Copyright (C) 2014 Yibo Cai
 */
#pragma once

/* XXX */
#define _TT_PI	3.14159265

/* Swap, may suffer side effects */
#define __tt_swap(a, b)				\
	do {					\
		__typeof__(a) xyz_t = (a);	\
		(a) = (b);			\
		(b) = xyz_t;			\
	} while (0)

/* Select key handlers (swap, set) */
int _tt_key_select(struct tt_key *num);
