/* Complex number
 *
 * Copyright (C) 2014 Yibo Cai
 */
#pragma once

/* Multiply
 * - Input and output may share same buffer
 */
static inline void tt_complex_mul(double *out,
		const double *in1, const double *in2)
{
	/* (a+bi)(c+di) */
	double ac = in1[0] * in2[0];
	double bd = in1[1] * in2[1];

#if 0
	/* This "smart" way is in fact slower than naive implementation */
	out[1] = (in1[0] + in1[1]) * (in2[0] + in2[1]) - ac - bd;
#else
	out[1] = in1[0] * in2[1] + in2[0] * in1[1];
#endif
	out[0] = ac - bd;
}

/* Add/Sub
 * - Input and output may share same buffer
 */
static inline void tt_complex_add(double *out,
		const double *in1, const double *in2)
{
	out[0] = in1[0] + in2[0];
	out[1] = in1[1] + in2[1];
}
static inline void tt_complex_sub(double *out,
		const double *in1, const double *in2)
{
	out[0] = in1[0] - in2[0];
	out[1] = in1[1] - in2[1];
}

static inline void tt_complex_set(double *out, const double *in)
{
	out[0] = in[0];
	out[1] = in[1];
}
