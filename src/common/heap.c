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

	if (heap->type == TT_HEAP_MIN) {
		tt_debug("Minimal heap initialized");
		heap->_heapify = tt_heap_heapify_min;
	} else {
		tt_debug("Maximal heap initialized");
		heap->_heapify = tt_heap_heapify_max;
	}

	heap->__heaplen = 0;

	return 0;
}
