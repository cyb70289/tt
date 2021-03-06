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

#define _tt_barrier()	__asm__ __volatile__("": : :"memory")
#define _tt_sync()	__sync_synchronize()

/* Select key handlers (swap, set) */
struct tt_key;
int _tt_key_select(struct tt_key *num);

/* Count decimal digits */
int _tt_digits(int n);
int _tt_digits_ll(long long int n);

/* String to integer */
int _tt_atoi(const char *str, int *i);

/* Rounding */
int _tt_round(int odd, uint dig, int rnd);

/* Bit reversal */
uint _tt_bitrev(uint n, int bits);

/* Pesudo random number */
void _tt_srand(uint seed);
uint _tt_rand(void);
