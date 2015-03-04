/* General heap implementation
 *
 * Copyright (C) 2014 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/heap.h>
#include "common.h"

/* Heap with N elements
 * - Leaf elements: [N/2] ~ [N-1]
 * - Child of element[i]: left - [i*2+1], right - [i*2+2]
 * - Parent of element[i]: [(i-1)/2]
 */

/* Reverse compare */
static int tt_heap_rcmp(const struct tt_key *num,
		const void *v1, const void *v2)
{
	return -num->cmp(num, v1, v2);
}

/* Heapify [index], taken [2*index+1] and [2*index+2] already heapified */
static inline void tt_heap_heapify_internal(
		struct tt_heap *heap, int index,
		int (*cmp)(const struct tt_key *, const void *, const void*))
{
	while (index < heap->__heaplen / 2) {
		/* Compare and swap with [2*index+1], [2*index+2] */
		void *iptr = heap_ptr(heap, index);
		index = index * 2 + 1;	/* Left child */
		void *lptr = heap_ptr(heap, index);
		void *rptr = lptr + heap->num.size;

		void *swaptr = iptr;
		if (cmp(&heap->num, swaptr, lptr) < 0)
			swaptr = lptr;
		if (index < heap->__heaplen - 1 &&
				cmp(&heap->num, swaptr, rptr) < 0) {
			swaptr = rptr;
			index++;	/* Right child */
		}
		if (swaptr != iptr)
			heap->num.swap(&heap->num, iptr, swaptr);
		else
			break;
	}
}

static void tt_heap_heapify_min(struct tt_heap *heap, int index)
{
	tt_heap_heapify_internal(heap, index, tt_heap_rcmp);
}

static void tt_heap_heapify_max(struct tt_heap *heap, int index)
{
	tt_heap_heapify_internal(heap, index, heap->num.cmp);
}

/* Rearrange element[index] to maintain heap property */
static inline void tt_heap_adjust_internal(
		struct tt_heap *heap, int index,
		int (*cmp)(const struct tt_key *, const void *, const void*))
{
	/* Buffer for one element */
	char tmp[heap->num.size] __align(8);	/* C99 VLA, huha! */
	void *v = heap_ptr(heap, index);
	heap->num._set(&heap->num, tmp, v);

	while (index > 0) {
		int parent = (index - 1) / 2;
		void *pv = heap_ptr(heap, parent);
		if (cmp(&heap->num, tmp, pv) <= 0)
			break;
		heap->num._set(&heap->num, v, pv);
		v = pv;
		index = parent;
	}

	heap->num._set(&heap->num, v, tmp);
}

static void tt_heap_adjust_min(struct tt_heap *heap, int index)
{
	tt_heap_adjust_internal(heap, index, tt_heap_rcmp);
}

static void tt_heap_adjust_max(struct tt_heap *heap, int index)
{
	tt_heap_adjust_internal(heap, index, heap->num.cmp);
}

/* Adjust an existing element */
int tt_heap_adjust(struct tt_heap *heap, int index, const void *v)
{
	tt_assert(index < heap->__heaplen);
	/* Verify new value
	 * max-heap: increase, min-heap: decrease
	 */
	int d = heap->num.cmp(&heap->num, heap_ptr(heap, index), v);
	if (d == 0)
		return 0;
	if ((heap->htype == TT_HEAP_MAX && d > 0) ||
			(heap->htype == TT_HEAP_MIN && d < 0)) {
		tt_error("Invalid new value");
		return TT_EINVAL;
	}

	heap->_adjust(heap, index);
	return 0;
}

/* Get and remove head element */
int tt_heap_extract(struct tt_heap *heap, void *v)
{
	if (heap->__heaplen == 0)
		return TT_EUNDERFLOW;
	heap->num._set(&heap->num, v, heap->data);

	heap->__heaplen--;
	if (heap->__heaplen) {
		heap->num._set(&heap->num, heap->data,
				heap_ptr(heap, heap->__heaplen));
		tt_heap_heapify(heap, 0);
	}

	return 0;
}

int tt_heap_init(struct tt_heap *heap)
{
	int ret = _tt_key_select(&heap->num);
	if (ret)
		return ret;

	if (heap->htype == TT_HEAP_MIN) {
		tt_debug("Min-heap initialized");
		heap->_heapify = tt_heap_heapify_min;
		heap->_adjust = tt_heap_adjust_min;
	} else {
		tt_debug("Max-heap initialized");
		heap->_heapify = tt_heap_heapify_max;
		heap->_adjust = tt_heap_adjust_max;
	}

	heap->__heaplen = 0;

	return 0;
}
