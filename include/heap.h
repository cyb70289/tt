/* Maxiam and minimum heap
 *
 * Copyright (C) 2014 Yibo Cai
 */
#pragma once

enum tt_heap_type {
	TT_HEAP_MAX,
	TT_HEAP_MIN,
};

struct tt_heap {
	struct tt_num		num;	/* Base number object */

	void	*data;	/* Data array */
	uint	cap;	/* Capacity (max element count) */
	enum tt_heap_type	htype;	/* Max or Min */

	uint	__heaplen;	/* Heapified subarray length */
	void	(*_heapify)(struct tt_heap *heap, int index);
	void	(*_adjust)(struct tt_heap *heap, int index);
};

/* Get address of i-th element */
#define heap_ptr(h, i)	((h)->data + (i) * (h)->num.size)

int tt_heap_init(struct tt_heap *heap);

/* Adjust data[index] to keep heap property,
 * taken that children of [index] are already heapified
 */
static inline void tt_heap_heapify(struct tt_heap *heap, int index)
{
	heap->_heapify(heap, index);
}

/* Heapify data[0] ~ data[len-1] */
static inline void tt_heap_build(struct tt_heap *heap, uint len)
{
	tt_assert(len <= heap->cap);
	heap->__heaplen = len;
	for (int i = len / 2 - 1; i >= 0; i--)
		tt_heap_heapify(heap, i);
}

/* Adjust an existing element */
int tt_heap_adjust(struct tt_heap *heap, int index, const void *v);

/* Insert a new element */
static inline int tt_heap_insert(struct tt_heap *heap, const void *v)
{
	if (heap->__heaplen == heap->cap)
		return -EOVERFLOW;
	heap->num._set(&heap->num, heap_ptr(heap, heap->__heaplen), v);
	heap->__heaplen++;
	heap->_adjust(heap, heap->__heaplen - 1);
	return 0;
}

/* Get and remove head element */
int tt_heap_extract(struct tt_heap *heap, void *v);
