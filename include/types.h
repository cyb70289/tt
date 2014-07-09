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
