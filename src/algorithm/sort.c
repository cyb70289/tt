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
#include <heap.h>
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
static int tt_sort_insert(struct tt_sort_input *input,
		struct tt_sort_stat *stat)
{
	/* Allocate buffer for one temporary element */
	void *tmp = malloc(input->size);
	if (!tmp)
		return -ENOMEM;
	stat->space += 1;

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
	return 0;
}

/* Merge sort (non-recursive)
 * - Time: O(n*logn)
 * - Space: O(n)
 */
static int tt_sort_merge(struct tt_sort_input *input,
		struct tt_sort_stat *stat)
{
	/* Allocate copy buffer */
	void *tmpbuf = malloc(input->count * input->size);
	if (!tmpbuf)
		return -ENOMEM;
	stat->space += input->count;

	int l = 1;	/* subarray length */
	while (l < input->count) {
		/* Merge [0]~[l-1] with [l]~[2l-1],
		 *       [2l]~[3l-1] with [3l]~[4l-1], ...
		 */
		int head = 0, mid, end;
		do {
			mid = head + l - 1;
			end = tt_min(mid + l, input->count - 1);

			/* Merge [head]~[mid] with [mid+1]~[end] */
			int p = head, p1 = head, p2 = mid + 1;
			void *tmptr = tmpbuf;
			void *ptr1 = dataptr(input, p1);
			void *ptr2 = dataptr(input, p2);
			while (p <= end) {
				bool la = p2 > end;	/* Pick left array */
				if (!la) {
					if (p1 > mid) {
						la = false;
					} else {
						la = input->cmp(ptr1, ptr2) < 0;
						stat->time++;	/* Compare */
					}
				}

				if (la) {
					input->_set(tmptr, ptr1, input->size);
					p1++;
					ptr1 += input->size;
				} else {
					input->_set(tmptr, ptr2, input->size);
					p2++;
					ptr2 += input->size;
				}
				p++;
				tmptr += input->size;
				stat->time++;	/* Set */
			}
			memcpy(dataptr(input, head), tmpbuf,
					(end - head + 1) * input->size);
			stat->time += (end - head + 1);	/* Set */

			head = end + 1;
		} while (head < input->count);

		l <<= 1;
	}

	free(tmpbuf);
	return 0;
}

/* Heap sort (non-recursive)
 * Time: O(n*logn)
 * Space: in-place
 */
static int tt_sort_heap(struct tt_sort_input *input,
		struct tt_sort_stat *stat)
{
	struct tt_heap heap = {
		.data	= input->data,
		.count	= input->count,
		.size	= input->size,
		.type	= TT_HEAP_MAX,
		.cmp	= input->cmp,
		.swap	= input->swap,
	};

	int ret = tt_heap_init(&heap);
	if (ret)
		return ret;

        /* Build heap */
	heap.__heaplen = heap.count;
	for (int i = heap.count / 2 - 1; i >= 0; i--)
		tt_heap_heapify(&heap, i);

	tt_assert(heap.__heaplen == heap.count);
	void *head = heap.data;
	heap.__heaplen--;
	void *end = dataptr(&heap, heap.__heaplen);
	for (; heap.__heaplen >= 1; heap.__heaplen--) {
		/* NOTE: Heap length is __heaplen+1 */
		/* Swap [0] with [__heaplen] */
		heap.swap(head, end);
		end -= heap.size;
		/* [__heaplen] is OK, let's heapify [0] ~ [__heaplen-1] */
		tt_heap_heapify(&heap, 0);
	}

	return 0;
}

static int (*tt_sort_internal[])(struct tt_sort_input *,
		struct tt_sort_stat *stat) = {
	[TT_SORT_INSERT]	= tt_sort_insert,
	[TT_SORT_MERGE]		= tt_sort_merge,
	[TT_SORT_HEAP]		= tt_sort_heap,
/*	[TT_SORT_QUICK]		= tt_sort_quick,*/
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
	int ret = tt_sort_internal[input->alg](input, &stat);

	tt_debug("time: %u, space: %u", stat.time, stat.space);

	return ret;
}
