/* Matrix
 *
 * Copyright (C) 2014 Yibo Cai
 */
#pragma once

/* Matrix */
struct tt_mtx {
	int rows;
	int cols;
	tt_float *v;
};

/* Matrix multiplication
 * [mi1] * [mi2] = [mo]
 */
int tt_mtx_mul(const struct tt_mtx *mi1, const struct tt_mtx *mi2,
		struct tt_mtx *mo);

/* Gauss-Jordan elimination
 * - Resolve linear equations: [ma].[x] = [mb]
 * - On return, [mb] holds [x], [ma] is ruined
 */
int tt_mtx_gaussj(struct tt_mtx *ma, struct tt_mtx *mb);
