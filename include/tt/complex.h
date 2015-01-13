/* Complex number
 *
 * Copyright (C) 2014 Yibo Cai
 */
#pragma once

/* Multiply
 * - Input and output may share same buffer
 */
static void tt_complex_mul(const tt_float *in1, const tt_float *in2,
		tt_float *out)
{
	tt_float r = in1[0] * in2[0] - in1[1] * in2[1];
	tt_float i = in1[0] * in2[1] + in1[1] * in2[0];

	out[0] = r;
	out[1] = i;
}
