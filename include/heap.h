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
	void	*data;
	uint	count;	/* Element count */
	uint	size;	/* Byte size of each element */
	enum tt_heap_type	type;	/* Max or Min */
	int	(*cmp)(const void *v1, const void *v2);
	void	(*swap)(void *v1, void *v2);
	void	(*_set)(void *, const void *, uint);
	void	(*_heapify)(struct tt_heap *heap, int index);
	uint	__heaplen;	/* Heapified subarray length */
};

int tt_heap_init(struct tt_heap *heap);

static inline void tt_heap_heapify(struct tt_heap *heap, int index)
{
	heap->_heapify(heap, index);
}
