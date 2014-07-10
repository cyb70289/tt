/* types.h
 *
 * Copyright (C) 2014 Yibo Cai
 */
#pragma once

#ifdef CONFIG_DOUBLE
typedef double	tt_float;
#else
typedef float	tt_float;
#endif

typedef unsigned char	uchar;
typedef unsigned short	ushort;
typedef unsigned int	uint;

enum tt_num_type {
	TT_NUM_SIGNED,		/* signed integer */
	TT_NUM_UNSIGNED,	/* unsigned integer */
	TT_NUM_FLOAT,		/* floating point */
	TT_NUM_USER,		/* user defined */
};

/* Number object */
struct tt_num {
	enum tt_num_type	type;	/* Number type */
	uint	size;	/* Element size in bytes */
	int	(*cmp)(const struct tt_num *num,
			const void *v1, const void *v2);/* Compare */
	void	(*swap)(const struct tt_num *num,
			void *v1, void *v2);		/* Swap */

	void	(*_set)(const struct tt_num *num,
			void *dst, const void *src);	/* Set value */
};
