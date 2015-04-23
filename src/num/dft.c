/* Non-optimized DFT & IDFT
 *
 * Copyright (C) 2014 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/num/dft.h>
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
int tt_dft(double (*out)[2], double (*in)[2], int N)
{
	tt_assert(N);

	double em = 0;
	double edelta = 2 * _TT_PI / N;
	for (int m = 0; m < N; m++) {
		out[m][0] = out[m][1] = 0;

		double emn = 0;
		for (int n = 0; n < N; n++) {
			double e[2] = { cos(emn), -sin(emn) };
			double t[2];
			tt_complex_mul(t, in[n], e);
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
int tt_idft(double (*out)[2], double (*in)[2], int N)
{
	tt_assert(N);

	double en = 0;
	double edelta = 2 * _TT_PI / N;
	for (int n = 0; n < N; n++) {
		out[n][0] = out[n][1] = 0;

		double enm = 0;
		for (int m = 0; m < N; m++) {
			double e[2] = { cos(enm), sin(enm) };
			double t[2];
			tt_complex_mul(t, in[m], e);
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
