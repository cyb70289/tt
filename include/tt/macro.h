/* macro.h
 *
 * Copyright (C) 2014 Yibo Cai
 */
#pragma once

/* No side effects */
#define tt_max(a, b)				\
	({					\
		__typeof__(a) xyz_a = (a);	\
		__typeof__(b) xyz_b = (b);	\
		xyz_a > xyz_b ? xyz_a : xyz_b;	\
	})

#define tt_min(a, b)				\
	({					\
		__typeof__(a) xyz_a = (a);	\
		__typeof__(b) xyz_b = (b);	\
		xyz_a < xyz_b ? xyz_a : xyz_b;	\
	})

#define ARRAY_SIZE(x)	(sizeof(x) / sizeof((x)[0]))

#define offset_of(TYPE, MEMBER)		((size_t) &((TYPE *)0)->MEMBER)
#define container_of(ptr, type, member)	({				\
	const __typeof__( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offset_of(type,member) );})

#define __likely(x)	(__builtin_expect(!!(x), 1))
#define __unlikely(x)	(__builtin_expect(!!(x), 0))
#define __weak		__attribute__ ((weak))
#define __align(n)	__attribute__ ((aligned (n)))
