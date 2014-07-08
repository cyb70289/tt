/* sort.h
 *
 * Copyright (C) 2014 Yibo Cai
 */
#pragma once

enum tt_sort_alg {
	TT_SORT_INSERT,
	TT_SORT_MERGE,
	TT_SORT_HEAP,
	TT_SORT_QUICK,

	TT_SORT_MAX,
};

/* Data array to be sorted */
struct tt_sort_input {
	void	*data;	/* Array to be sorted */
	uint	count;	/* Element count */
	uint	size;	/* Elememt size */
	enum tt_num_type	type;		/* Element numerical type */
	enum tt_sort_alg	alg;		/* Sorting algorithm */
	int	(*cmp)(const void *v1, const void *v2);	/* Compare routine */
	void	(*swap)(void *v1, void *v2);	/* Swap routine */
	void	(*_set)(void *, const void *, uint);	/* Set value */
};

int tt_sort(struct tt_sort_input *input);
