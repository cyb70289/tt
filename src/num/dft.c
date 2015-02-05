/* Non-optimized DFT & IDFT
 *
 * Copyright (C) 2014 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/num/dsp.h>
#include <tt/num/complex.h>
#include "num.h"

#include <math.h>

/*
 *         N-1
 *        ----             2*PI*m
 *        \             -j*------*n
 * X(m)=  /    x(n) * e      N
 *        ----
 *        n = 0
 *
 * in, out: [real1, image1, real2, image2, ..., realN, imageN]
 */
int tt_dft(tt_float (*in)[2], tt_float (*out)[2], int N)
{
	tt_assert(N);

	tt_float em = 0;
	tt_float edelta = 2 * _TT_PI / N;
	for (int m = 0; m < N; m++) {
		out[m][0] = out[m][1] = 0;

		tt_float emn = 0;
		for (int n = 0; n < N; n++) {
			tt_float e[2] = { cos(emn), -sin(emn) };
			tt_float t[2];
			tt_complex_mul(in[n], e, t);
			out[m][0] += t[0];
			out[m][1] += t[1];

			emn += em;
		}

		em += edelta;
	}

	return 0;
}

/*
 *            N-1
 *           ----             2*PI*n
 *        1  \             j*-------*m
 * x(n)= --- /    X(m) * e      N
 *        N  ----
 *           m = 0
 *
 * in, out: [real1, image1, real2, image2, ..., realN, imageN]
 */
int tt_idft(tt_float (*in)[2], tt_float (*out)[2], int N)
{
	tt_assert(N);

	tt_float en = 0;
	tt_float edelta = 2 * _TT_PI / N;
	for (int n = 0; n < N; n++) {
		out[n][0] = out[n][1] = 0;

		tt_float enm = 0;
		for (int m = 0; m < N; m++) {
			tt_float e[2] = { cos(enm), sin(enm) };
			tt_float t[2];
			tt_complex_mul(in[m], e, t);
			out[n][0] += t[0];
			out[n][1] += t[1];

			enm += en;
		}
		out[n][0] /= N;
		out[n][1] /= N;

		en += edelta;
	}

	return 0;
}
