/* Matrix
 *
 * Copyright (C) 2014 Yibo Cai
 */
#pragma once

/* Matrix */
struct tt_mtx {
	int rows;
	int cols;
	double *v;
};

/* Matrix multiplication
 * [mo] = [mi1] * [mi2]
 */
int tt_mtx_mul(struct tt_mtx *mo,
		const struct tt_mtx *mi1, const struct tt_mtx *mi2);

/* Gauss-Jordan elimination
 * - Resolve linear equations: [ma].[x] = [mb]
 * - On return, [mb] holds [x], [ma] is ruined
 */
int tt_mtx_gaussj(struct tt_mtx *ma, struct tt_mtx *mb);
