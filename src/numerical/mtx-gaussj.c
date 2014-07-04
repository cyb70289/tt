/* Gauss-Jordan elimination
 *
 * Copyright (C) 2014 Yibo Cai
 */
#include <tt.h>
#include <math.h>

#include <matrix.h>
#include <_common.h>
#include <_matrix.h>

/* Gauss-Jordan elimination
 * - Resolve linear equations: [ma].[x] = [mb]
 * - On return, [mb] holds [x], [ma] is ruined
 * Algorithm complexity
 * - Space: O(n)
 * - Time: O(n^3)
 */
int tt_mtx_gaussj(struct tt_mtx *ma, struct tt_mtx *mb)
{
	int ret = 0;
	int i, j;
	int *status_buf;
	int *row_processed, *col_processed;
	int *switchto;

	tt_assert(ma->rows == ma->cols);
	tt_assert(ma->rows == mb->rows);

	/* Alloc memory at once in a larger buffer */
	status_buf = calloc(3 * ma->rows, sizeof(int));
	if (status_buf == NULL)
		return -ENOMEM;
	row_processed = status_buf;
	col_processed = row_processed + ma->rows;
	switchto = col_processed + ma->rows;

	for (int row = 0; row < ma->rows; row++) {
		/* Find pivot */
		int rowp = 0, colp = 0;
		tt_float max = 0.0f;
		for (i = 0; i < ma->rows; i++) {
			if (row_processed[i])
				continue;
			for (j = 0; j < ma->cols; j++) {
				if (col_processed[j])
					continue;
				tt_float v = fabs(*_tt_mtx_el(ma, i, j));
				if (v > max) {
					max = v;
					rowp = i;
					colp = j;
				}
			}
		}
		if (_tt_is_zero(max)) {
			ret = -ESINGULAR;	/* Singular matrix */
			goto out;
		}

		/* Pivot is at (rowp, colp). We needn't switch it physically
		 * to (row, row), just identity and eliminate inplace.
		 * Remember the switched orders and restore at the end.
		 */
		switchto[rowp] = colp;

		/* Identity row */
		max = *_tt_mtx_el(ma, rowp, colp);
		_tt_vec_div_sca(_tt_mtx_row(ma, rowp), ma->cols, max);
		_tt_vec_div_sca(_tt_mtx_row(mb, rowp), mb->cols, max);

		/* Eliminate column */
		for (i = 0; i < ma->rows; i++) {
			if (i == rowp)
				continue;

			/* Process coefficient matrix [ma] */
			tt_float *v = _tt_mtx_row(ma, i);
			tt_float *vp = _tt_mtx_row(ma, rowp);
			tt_float _1 = v[colp];
			for (j = 0; j < ma->cols; j++) {
				/* Ignore processed and current column */
				if (col_processed[j] || j == colp)
					continue;
				v[j] -= vp[j] * _1;
			}

			/* Process result matrix [mb] */
			_tt_vec_add_vec_mul_sca(_tt_mtx_row(mb, i),
					_tt_mtx_row(mb, rowp), mb->cols, -_1);
		}

		row_processed[rowp] = col_processed[colp] = 1;
	}

	/* Rows are out of order now. We need to switch them back:
	 * row i should be switch to row switchto[i]
	 */
	for (i = 0; i < mb->rows; i++) {
		j = switchto[i];
		if (i != j) {
			_tt_vec_swap(_tt_mtx_row(mb, i),
					_tt_mtx_row(mb, j), mb->cols);
			__tt_swap(switchto[j], switchto[i]);
		}
	}

out:
	free(status_buf);
	return ret;
}
