/* General sorting algorithms
 *
 * Copyright (C) 2014 Yibo Cai
 *
 * General programming is hard, especially for C. I struggled much and finally
 * rejected C++ template because it still couldn't meet all conditions, and I
 * really dislike C++.
 * Don't blame me on performance loss of this messy code. I think I'm correct
 * in a big picture.
 */
#include <tt.h>
#include <string.h>

#include <sort.h>
#include <_common.h>

/* algorithm complexity statistics */
struct tt_sort_stat {
	uint	time;	/* Time cost */
	uint	space;	/* Extra space */
};

/* &(input->data[i]) */
#define dataptr(input, i)	((input)->data + (i) * (input)->size)

/* Insert sort
 * - Time: O(n^2)
 * - Space: in-place
 * - Insert sort beats O(n*logn) algorithms on small array
 */
static void tt_sort_insert(struct tt_sort_input *input,
		struct tt_sort_stat *stat)
{
	/* Allocate buffer for one temporary element */
	void *tmp = malloc(input->size);

	void *p = input->data;
	for (int i = 1; i < input->count; i++) {
		p += input->size;
		/* tmp = data[i] */
		input->_set(tmp, p, input->size);
		/* Insert data[i] into sorted list data[0] ~ data[i-1] */
		void *q = p - input->size;
		for (int j = i - 1; j >= 0; q -= input->size, j--) {
			stat->time++;	/* Compare */
			/* if (tmp < data[j]) */
			if (input->cmp(tmp, q) < 0) {
				/* data[j+1] = data[j] */
				input->_set(q + input->size, q, input->size);
				stat->time++;	/* Set */
			} else {
				break;
			}
		}
		/* data[j+1] = *tmp */
		input->_set(q + input->size, tmp, input->size);
		stat->time++;	/* Set */
	}

	free(tmp);
}

static void (*tt_sort_internal[])(struct tt_sort_input *,
		struct tt_sort_stat *stat) = {
	[TT_SORT_INSERT]	= tt_sort_insert,
/*	[TT_SORT_MERGE]		= tt_sort_merge,
	[TT_SORT_HEAP]		= tt_sort_heap,
	[TT_SORT_QUICK]		= tt_sort_quick,*/
};

int tt_sort(struct tt_sort_input *input)
{
	tt_assert(input->alg >= 0 && input->alg < TT_SORT_MAX);

	/* Set default swap, compare, set routines */
	if (!input->swap) {
		input->swap = _tt_swap_select(input->size);
		if (!input->swap) {
			tt_error("No default swap routine");
			return -EPARAM;
		}
	}
	if (!input->cmp) {
		input->cmp = _tt_cmp_select(input->size, input->type);
		if (!input->cmp) {
			tt_error("No default compare routine");
			return -EPARAM;
		}
	}
	input->_set = _tt_set_select(input->size);

	struct tt_sort_stat stat;
	memset(&stat, 0, sizeof(struct tt_sort_stat));
	tt_sort_internal[input->alg](input, &stat);

	tt_debug("time: %u, space: %u", stat.time, stat.space);

	return 0;
}
