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
	double r = in1[0] * in2[0] - in1[1] * in2[1];
	double i = in1[0] * in2[1] + in1[1] * in2[0];

	out[0] = r;
	out[1] = i;
}
