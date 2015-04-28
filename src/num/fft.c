/* Fast Fourier Transform
 *
 * Copyright (C) 2015 Yibo Cai
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

/* TODO: keep lookup table for some time */
static double (*get_wn_tbl(int N, int *delta))[2]
{
	if (N > WN_PTS) {
		tt_info("Points > %d, new lookup table created.", WN_PTS);
		*delta = 1;
		return gen_wn_tbl(N);
	} else {
		*delta = WN_PTS / N;
		return wn_ftbl;
	}
}

static void put_wn_tbl(double (*tbl)[2])
{
	if (tbl != wn_ftbl)
		free(tbl);
}

/* Evaluate FFT at each stage (decimation in time)
 *
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
		double (*tbl)[2], int delta)
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
			tt_complex_mul(x[idx], x[idx], tbl[tbl_idx]);
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

static void ifft_stage(double (*x)[2], int stage, uint N,
		double (*tbl)[2], int delta)
{
	const int idx_step = 1 << (stage - 1);
	int idx;
	double tmp[2];

	const int tbl_step = delta * (N >> stage);
	idx = idx_step;
	for (int i = 0; i < (N >> stage); i++) {
		int tbl_idx = 0;
		for (int j = 0; j < idx_step; j++) {
			tmp[0] = tbl[tbl_idx][0];
			tmp[1] = -tbl[tbl_idx][1];
			tt_complex_mul(x[idx], x[idx], tmp);
			idx++;
			tbl_idx += tbl_step;
		}
		idx += idx_step;
	}

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

static int do_stage(double (*out)[2], double (*in)[2], uint N,
		void (*stage)(double (*)[2], int, uint, double (*)[2], int))
{
	double (*tbl)[2];
	int delta;

	tbl = get_wn_tbl(N, &delta);
	if (!tbl)
		return TT_ENOMEM;

	/* Bit reversal */
	const int nb = __builtin_ctz(N);
	for (int i = 0; i < N ; i++)
		tt_complex_set(out[i], in[_tt_bitrev(i, nb)]);

	for (int i = 1; i <= nb; i++)
		stage(out, i, N, tbl, delta);

	put_wn_tbl(tbl);

	return 0;
}

int tt_fft(double (*out)[2], double (*in)[2], uint N)
{
	/* Check if N is 2^n */
	if (N <= 1 || (N & (N-1))) {
		tt_warn("%d is not power of 2, fallback to DFT", N);
		return tt_dft(out, in, N);
	}

	return do_stage(out, in, N, fft_stage);
}

int tt_ifft(double (*out)[2], double (*in)[2], uint N)
{
	if (N <= 1 || (N & (N-1))) {
		tt_warn("%d is not power of 2, fallback to IDFT", N);
		return tt_idft(out, in, N);
	}

	int ret = do_stage(out, in, N, ifft_stage);
	if (ret)
		return ret;

	/* Divide N */
	for (int i = 0; i < N; i++) {
		out[i][0] /= N;
		out[i][1] /= N;
	}

	return 0;
}

int tt_fft_real(double (*out)[2], const double *in, uint N)
{
	if (N <= 1 || (N & (N-1))) {
		tt_warn("%d is not power of 2, fallback to DFT", N);
		return tt_dft_real(out, in, N);
	}

	double (*tbl)[2];
	int delta;

	tbl = get_wn_tbl(N, &delta);
	if (!tbl)
		return TT_ENOMEM;

	const int nb = __builtin_ctz(N);
	for (int i = 0; i < N ; i++) {
		out[i][0] = in[_tt_bitrev(i, nb)];
		out[i][1] = 0;
	}

	for (int i = 1; i <= nb; i++)
		fft_stage(out, i, N, tbl, delta);

	put_wn_tbl(tbl);

	return 0;
}
