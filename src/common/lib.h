/* Common libs
 *
 * Copyright (C) 2014 Yibo Cai
 */
#pragma once

#define _TT_PI	3.1415926535897932

/* Maximum, Mininum without side effects */
#define _tt_max(a, b)				\
	({					\
		__typeof__(a) xyz_a = (a);	\
		__typeof__(b) xyz_b = (b);	\
		xyz_a > xyz_b ? xyz_a : xyz_b;	\
	})

#define _tt_min(a, b)				\
	({					\
		__typeof__(a) xyz_a = (a);	\
		__typeof__(b) xyz_b = (b);	\
		xyz_a < xyz_b ? xyz_a : xyz_b;	\
	})

/* Swap, may suffer side effects */
#define __tt_swap(a, b)				\
	do {					\
		__typeof__(a) xyz_t = (a);	\
		(a) = (b);			\
		(b) = xyz_t;			\
	} while (0)

#define BIT(n)	(1U << (n))

#define ARRAY_SIZE(x)	(sizeof(x) / sizeof((x)[0]))

#define offset_of(TYPE, MEMBER)		((size_t) &((TYPE *)0)->MEMBER)
#define container_of(ptr, type, member)	({				\
	const __typeof__( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offset_of(type,member) );})

#define _tt_likely(x)	(__builtin_expect(!!(x), 1))
#define _tt_unlikely(x)	(__builtin_expect(!!(x), 0))
#define _tt_weak	__attribute__ ((weak))
#define _tt_align(n)	__attribute__ ((aligned (n)))

/* Select key handlers (swap, set) */
struct tt_key;
int _tt_key_select(struct tt_key *num);

/* Count decimal digits */
int _tt_digits(int n);
int _tt_digits_ll(long long int n);

/* String to integer */
int _tt_atoi(const char *str, int *i);

/* Add,sub and return carry */
uint _tt_add_uint(uint a, uint b, char *c);
uint _tt_sub_uint(uint a, uint b, char *c);

/* Rounding */
int _tt_round(int odd, uint dig, int rnd);

/* Memory management */
#include "mm/mm.h"
