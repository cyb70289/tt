/* Matrix operations
 *
 * Copyright (C) 2014 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/num/matrix.h>
#include "num.h"
#include "mtx.h"

/* Naive implementation of matrix multiplication
 * [mi1] * [mi2] = mo
 * Algorithm complexity
 * - Space: O(1)
 * - Time: O(n^3)
 * TODO: optimization
 */
int tt_mtx_mul(const struct tt_mtx *mi1, const struct tt_mtx *mi2,
		struct tt_mtx *mo)
{
	tt_assert(mi1->cols == mi2->rows);
	tt_assert(mo->rows == mi1->rows);
	tt_assert(mo->cols == mi2->cols);

	for (int i = 0; i < mi1->rows; i++) {
		tt_float *v1 = _tt_mtx_row(mi1, i);
		for (int j = 0; j < mi2->cols; j++) {
			tt_float o = 0.0f;
			tt_float *v2 = _tt_mtx_el(mi2, 0, j);
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
