/* Common numerical libs
 *
 * Copyright (C) 2014 Yibo Cai
 */
#pragma once

#include "../common/common.h"

/* TODO: needs check */
static inline bool _tt_is_zero(tt_float v)
{
	return v == 0.0f;
}

/* Swap [vector1] & [vector2]
 * TODO: optimization
 */
static inline void _tt_vec_swap(tt_float *vec1, tt_float *vec2, int n)
{
	for (int i = 0; i < n; i++)
		__tt_swap(vec1[i], vec2[i]);
}

/* [vector] = [vector] / sca
 * TODO: optimization
 */
static inline void _tt_vec_div_sca(tt_float *vec, int n, tt_float sca)
{
	if (_tt_is_zero(sca))
		tt_error("Divide by 0!");

	for (int i = 0; i < n; i++)
		vec[i] /= sca;
}

/* [vector1] = [vector1] + [vector2] * scalar
 * TODO: optimization
 */
static inline void _tt_vec_add_vec_mul_sca(tt_float *vec1, const tt_float *vec2,
		int n, tt_float sca)
{
	for (int i = 0; i < n; i++)
		vec1[i] += vec2[i] * sca;
}
