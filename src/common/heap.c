/* General heap implementation
 *
 * Copyright (C) 2014 Yibo Cai
 */
#include <tt.h>
#include <heap.h>
#include <_common.h>

/* Heap with N elements
 * - Leaf elements: [N/2] ~ [N-1]
 * - Child of element[i]: left - [i*2+1], right - [i*2+2]
 * - Parent of element[i]: [(i-1)/2]
 */

#define dataptr(heap, i)	((heap)->data + (i) * (heap)->size)

/* Heapify [index], taken [2*index+1] and [2*index+2] already heapified */
static void tt_heap_heapify_min(struct tt_heap *heap, int index)
{
	while (index < heap->__heaplen / 2) {
		/* Compare and swap with [2*index+1], [2*index+2] */
		void *iptr = dataptr(heap, index);
		index = index * 2 + 1;	/* Left child */
		void *lptr = dataptr(heap, index);
		void *rptr = lptr + heap->size;

		void *swaptr = iptr;
		if (heap->cmp(swaptr, lptr) > 0)
			swaptr = lptr;
		if (index < heap->__heaplen - 1 && heap->cmp(swaptr, rptr) > 0) {
			swaptr = rptr;
			index++;	/* Right child */
		}
		if (swaptr != iptr)
			heap->swap(iptr, swaptr);
		else
			break;
	}
}

static void tt_heap_heapify_max(struct tt_heap *heap, int index)
{
	while (index < heap->__heaplen / 2) {
		/* Compare and swap with [2*index+1], [2*index+2] */
		void *iptr = dataptr(heap, index);
		index = index * 2 + 1;	/* Left child */
		void *lptr = dataptr(heap, index);
		void *rptr = lptr + heap->size;

		void *swaptr = iptr;
		if (heap->cmp(swaptr, lptr) < 0)
			swaptr = lptr;
		if (index < heap->__heaplen - 1 && heap->cmp(swaptr, rptr) < 0) {
			swaptr = rptr;
			index++;	/* Right child */
		}
		if (swaptr != iptr)
			heap->swap(iptr, swaptr);
		else
			break;
	}
}

/* Adjust heap element */
static void tt_heap_adjust_min(struct tt_heap *heap, int index)
{
	void *v = dataptr(heap, index);
	while (index > 0) {
		int parent = (index - 1) / 2;
		void *pv = dataptr(heap, parent);
		if (heap->cmp(v, pv) >= 0)
			break;
		heap->swap(v, pv);
		index = parent;
		v = pv;
	}
}

static void tt_heap_adjust_max(struct tt_heap *heap, int index)
{
	void *v = dataptr(heap, index);
	while (index > 0) {
		int parent = (index - 1) / 2;
		void *pv = dataptr(heap, parent);
		if (heap->cmp(v, pv) <= 0)
			break;
		heap->swap(v, pv);
		index = parent;
		v = pv;
	}
}

/* Adjust an existing element */
int tt_heap_adjust(struct tt_heap *heap, int index, const void *v)
{
	tt_assert(index < heap->__heaplen);
	/* Verify new value
	 * max-heap: increase, min-heap: decrease
	 */
	int d = heap->cmp(dataptr(heap, index), v);
	if (d == 0)
		return 0;
	if ((heap->htype == TT_HEAP_MAX && d > 0) ||
			(heap->htype == TT_HEAP_MIN && d < 0)) {
		tt_error("Invalid new value");
		return -EINVAL;
	}

	heap->_adjust(heap, index);
	return 0;
}

/* Get and remove head element */
int tt_heap_gethead(struct tt_heap *heap, void *v)
{
	if (heap->__heaplen == 0)
		return -EUNDERFLOW;
	heap->_set(v, heap->data, heap->size);

	heap->__heaplen--;
	if (heap->__heaplen) {
		heap->_set(heap->data,
			heap->data + heap->__heaplen * heap->size, heap->size);
		tt_heap_heapify(heap, 0);
	}

	return 0;
}

int tt_heap_init(struct tt_heap *heap)
{
	/* Set default swap, compare, set routines */
	if (!heap->swap) {
		heap->swap = _tt_swap_select(heap->size);
		if (!heap->swap) {
			tt_error("No default swap routine");
			return -EPARAM;
		}
	}
	if (!heap->cmp) {
		heap->cmp = _tt_cmp_select(heap->size, heap->type);
		if (!heap->cmp) {
			tt_error("No default compare routine");
			return -EPARAM;
		}
	}
	heap->_set = _tt_set_select(heap->size);

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
