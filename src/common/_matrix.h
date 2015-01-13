/* _matrix.h
 *
 * Copyright (C) 2014 Yibo Cai
 */
#pragma once

/* Get element m[row][col] */
static inline tt_float *_tt_mtx_el(const struct tt_mtx *m, int row, int col)
{
	tt_assert_fa(row < m->rows);
	tt_assert_fa(col < m->cols);

	return m->v + row * m->cols + col;
}

/* Get m[row][] pointer */
static inline tt_float *_tt_mtx_row(const struct tt_mtx *m, int row)
{
	tt_assert_fa(row < m->rows);

	return m->v + row * m->cols;
}
