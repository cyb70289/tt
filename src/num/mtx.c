/* Matrix operations
 *
 * Copyright (C) 2014 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/num/matrix.h>
#include "num.h"
#include "mtx.h"

/* Naive implementation of matrix multiplication
 * [mo] = [mi1] * [mi2]
 * Algorithm complexity
 * - Space: O(1)
 * - Time: O(n^3)
 * TODO: optimization
 */
int tt_mtx_mul(struct tt_mtx *mo,
		const struct tt_mtx *mi1, const struct tt_mtx *mi2)
{
	tt_assert(mi1->cols == mi2->rows);
	tt_assert(mo->rows == mi1->rows);
	tt_assert(mo->cols == mi2->cols);

	for (int i = 0; i < mi1->rows; i++) {
		double *v1 = _tt_mtx_row(mi1, i);
		for (int j = 0; j < mi2->cols; j++) {
			double o = 0.0f;
			double *v2 = _tt_mtx_el(mi2, 0, j);
			for (int k = 0; k < mi1->cols; k++) {
				o += (*v1) * (*v2);
				v1++;
				v2 += mi2->cols;
			}
			*_tt_mtx_el(mo, i, j) = o;
		}
	}

	return 0;
}
