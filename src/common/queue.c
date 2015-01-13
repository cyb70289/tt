/* General queue implementation
 *
 * Copyright (C) 2014 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/list.h>
#include <tt/queue.h>
#include "_common.h"

#include <string.h>

/* Static allocated queue: Ring buffer */
static int tt_queue_enque_array(struct tt_queue *queue, const void *e)
{
	if (queue->_count >= queue->cap) {
		tt_debug("Overflow");
		return -TT_EOVERFLOW;
	}
	memcpy(queue->_qtail, e, queue->size);
	queue->_qtail += queue->size;
	/* Wrap to head */
	if (queue->_qtail >= queue->_data + queue->cap * queue->size)
		queue->_qtail = queue->_data;

	queue->_count++;
	return 0;
}

static int tt_queue_deque_array(struct tt_queue *queue, void *e)
{
	if (queue->_count == 0) {
		tt_debug("Underflow");
		return -TT_EUNDERFLOW;
	}
	memcpy(e, queue->_qhead, queue->size);
	queue->_qhead += queue->size;
	/* Wrap to head */
	if (queue->_qhead >= queue->_data + queue->cap * queue->size)
		queue->_qhead = queue->_data;

	queue->_count--;
	return 0;
}

/* Dynamic linked queue */
static int tt_queue_enque_list(struct tt_queue *queue, const void *e)
{
	void *qe = malloc(sizeof(struct tt_list) + queue->size);
	if (!qe)
		return -TT_EOVERFLOW;
	memcpy(qe + sizeof(struct tt_list), e, queue->size);
	tt_list_add(qe, &queue->_head);

	queue->_count++;
	return 0;
}

static int tt_queue_deque_list(struct tt_queue *queue, void *e)
{
	if (queue->_count == 0) {
		tt_debug("Underflow");
		return -TT_EUNDERFLOW;
	}
	void *qe = queue->_head.next;
	memcpy(e, qe + sizeof(struct tt_list), queue->size);
	tt_list_del(qe);
	free(qe);

	queue->_count--;
	return 0;
}

int tt_queue_init(struct tt_queue *queue)
{
	if (queue->cap) {
		/* Allocate fixed array */
		queue->_data = malloc(queue->size * queue->cap);
		if (!queue->_data)
			return -TT_ENOMEM;
		queue->_qhead = queue->_qtail = queue->_data;
		queue->_enque = tt_queue_enque_array;
		queue->_deque = tt_queue_deque_array;
		tt_debug("Queue created, max elements = %u", queue->cap);
	} else {
		/* XXX: Allocate tt_list struct before element,
		 * the queue element only ensures 8-bytes alignment
		 */
		tt_assert(sizeof(struct tt_list) % 8 == 0);
		/* Initialize list head */
		tt_list_init(&queue->_head);
		queue->_enque = tt_queue_enque_list;
		queue->_deque = tt_queue_deque_list;
		tt_debug("Dynamic queue created");
	}

	queue->_count = 0;
	return 0;
}

void tt_queue_free(struct tt_queue *queue)
{
	if (queue->cap) {
		/* Free the array */
		free(queue->_data);
		queue->_data = NULL;
	} else {
		/* Free the list */
		struct tt_list *pos, *tmp;
		list_for_each_safe(pos, tmp, &queue->_head)
			free(pos);
	}

	tt_debug("Queue destroyed");
	queue->_count = 0;
}
