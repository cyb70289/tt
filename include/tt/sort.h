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
	struct tt_key num;

	void *data;		/* Element buffer */
	uint count;		/* Element count */
	enum tt_sort_alg alg;	/* Sorting algorithm */
};

/* Get address of i-th element */
#define sort_ptr(s, i)	((s)->data + (i) * (s)->num.size)

int tt_sort(struct tt_sort_input *input);
