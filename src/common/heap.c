/* General heap implementation
 *
 * Copyright (C) 2014 Yibo Cai
 */
#include <tt.h>
#include <heap.h>
#include <_common.h>

#define dataptr(heap, i)	((heap)->data + (i) * (heap)->size)

/* Heapify [index], taken [2*index+1] and [2*index+2] already heapified */
static void tt_heap_heapify_min(struct tt_heap *heap, int index)
{
	while (index < heap->_heaplen / 2) {
		/* Compare and swap with [2*index+1], [2*index+2] */
		void *iptr = dataptr(heap, index);
		index = index * 2 + 1;	/* Left child */
		void *lptr = dataptr(heap, index);
		void *rptr = lptr + heap->size;

		void *swaptr = iptr;
		if (heap->cmp(swaptr, lptr) > 0)
			swaptr = lptr;
		if (index < heap->_heaplen - 1 && heap->cmp(swaptr, rptr) > 0) {
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
	while (index < heap->_heaplen / 2) {
		/* Compare and swap with [2*index+1], [2*index+2] */
		void *iptr = dataptr(heap, index);
		index = index * 2 + 1;	/* Left child */
		void *lptr = dataptr(heap, index);
		void *rptr = lptr + heap->size;

		void *swaptr = iptr;
		if (heap->cmp(swaptr, lptr) < 0)
			swaptr = lptr;
		if (index < heap->_heaplen - 1 && heap->cmp(swaptr, rptr) < 0) {
			swaptr = rptr;
			index++;	/* Right child */
		}
		if (swaptr != iptr)
			heap->swap(iptr, swaptr);
		else
			break;
	}
}

int tt_heap_build(struct tt_heap *heap)
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

	/* Build heap */
	heap->_heaplen = heap->count;
        for (int i = heap->count / 2 - 1; i >= 0; i--)
		heap->_heapify(heap, i);

	return 0;
}
