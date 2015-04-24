/* Fast Fourier Transform
 *
 * Copyright (C) 2014 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/num/dft.h>
#include <tt/num/complex.h>
#include "num.h"

#include <math.h>

/* Fast lookup table (1/2 period, how about 1/8 period?) */
#define WN_PTS	8192	/* Must be 2^n */
static double (*wn_ftbl)[2];

static double (*gen_wn_tbl(int N))[2]
{
	double wn = 0;
	const double step = 2 * _TT_PI / N;
	double (*tbl)[2] = malloc(N / 2 * sizeof(*tbl));

	if (tbl == NULL) {
		tt_error("Out of memory");
		return NULL;
	}

	for (int i = 0; i < N / 4; i++) {
		tbl[i][0] = cos(wn);
		tbl[i][1] = -sin(wn);
		tbl[i+N/4][0] = tbl[i][1];
		tbl[i+N/4][1] = -tbl[i][0];
		wn += step;
	}

	return tbl;
}

static __attribute__ ((constructor)) void fft_init(void)
{
	wn_ftbl = gen_wn_tbl(WN_PTS);
	tt_debug("FFT lookup table created: %d points, %d KB",
			WN_PTS, WN_PTS / 2 * sizeof(*wn_ftbl) / 1024);
}

static __attribute__ ((destructor)) void fft_deinit(void)
{
	free(wn_ftbl);
}

/* Bit reversal
 * - 1 <= bits <= 31, N = 2^bits.
 * - http://www.katjaas.nl/bitreversal/bitreversal.html
 */
static uint bit_reverse(uint n, int bits, uint N)
{
	uint rev = n;
	uint cnt = bits - 1;

	for (n >>= 1; n; n >>= 1) {
		rev <<= 1;
		rev |= (n & 1);
		cnt--;
	}

	rev <<= cnt;
	rev &= (N - 1);

	return rev;
}

/* Evaluate FFT at each stage
 * stage1           stage2                  stage3...
 *---------o-----o-----------o-----------o------------
 *          \   /             \         /
 *           \ /               \       /
 *            X                 \     /
 *           / \                 \   /
 *          /   \                 \ /
 *---------o-----o-----------o-----X-----o-----------
 * W(N,0)    -1               \   / \   /
 *                             \ /   \ /
 *                              X     X
 *                             / \   / \
 *                            /   \ /   \
 *---------o-----o-----------o-----X-----o-----------
 *          \   /    W(N,0)       / \ -1
 *           \ /                 /   \
 *            X                 /     \
 *           / \               /       \
 *          /   \             /         \
 *---------o-----o-----------o-----------o-----------
 * W(N,0)    -1      W(N,1)           -1
 *
 *......
 */
static void fft_stage(double (*x)[2], int stage, uint N,
		double (*tbl)[2], int delta, bool inverse)
{
	const int idx_step = 1 << (stage - 1);
	int idx;
	double tmp[2];

	/* Multiply W(N,k) */
	const int tbl_step = delta * (N >> stage);
	idx = idx_step;
	for (int i = 0; i < (N >> stage); i++) {
		int tbl_idx = 0;
		for (int j = 0; j < idx_step; j++) {
			if (inverse) {
				tmp[0] = tbl[tbl_idx][0];
				tmp[1] = -tbl[tbl_idx][1];
				tt_complex_mul(x[idx], x[idx], tmp);
			} else {
				tt_complex_mul(x[idx], x[idx], tbl[tbl_idx]);
			}
			idx++;
			tbl_idx += tbl_step;
		}
		idx += idx_step;
	}

	/* Cross add/sub */
	idx = 0;
	for (int i = 0; i < (N >> stage); i++) {
		for (int j = 0; j < idx_step; j++) {
			tt_complex_add(tmp, x[idx], x[idx+idx_step]);
			tt_complex_sub(x[idx+idx_step], x[idx], x[idx+idx_step]);
			tt_complex_set(x[idx], tmp);
			idx++;
		}
		idx += idx_step;
	}
}

/* FFT: decimation in time
 * - in, out: [real1, image1, real2, image2, ..., realN, imageN]
 * - inverse: true - inverse fft
 * - TODO: keep lookup table for some time
 */
static int fft(double (*out)[2], double (*in)[2], uint N, bool inverse)
{
	double (*tbl)[2] = wn_ftbl;
	int delta = 1;

	/* Check if N is 2^n */
	if (N <= 1 || (N & (N-1))) {
		tt_warn("%d is not power of 2, fallback to DFT", N);
		return inverse ? tt_idft(in, out, N) : tt_dft(in, out, N);
	}

	/* Generate lookup table if default one cannot fit */
	if (N > WN_PTS) {
		tt_info("Points > %d, new lookup table generated", WN_PTS);
		tbl = gen_wn_tbl(N);
		if (tbl == NULL)
			return TT_ENOMEM;
	} else {
		delta = WN_PTS / N;
	}

	/* Bit reversal */
	const int nb = __builtin_ctz(N);
	for (int i = 0; i < N ; i++)
		tt_complex_set(out[i], in[bit_reverse(i, nb, N)]);

	for (int stage = 1; stage <= nb; stage++)
		fft_stage(out, stage, N, tbl, delta, inverse);

	if (tbl != wn_ftbl)
		free(tbl);

	/* Divide N if IFFT */
	if (inverse) {
		for (int i = 0; i < N; i++) {
			out[i][0] /= N;
			out[i][1] /= N;
		}
	}

	return 0;
}

int tt_fft(double (*out)[2], double (*in)[2], uint N)
{
	return fft(out, in, N, false);
}

int tt_ifft(double (*out)[2], double (*in)[2], uint N)
{
	return fft(out, in, N, true);
}
