/* General queue
 *
 * Copyright (C) 2014 Yibo Cai
 *
 * XXX: Queue elements only ensure 8-bytes alignment
 */
#pragma once

#include <list.h>

struct tt_queue {
	uint	cap;	/* Maxiam elements, 0 - dynamic */
	uint	size;	/* Byte size of each element */

	uint	_count;	/* Elements count */
	union {
		struct {
			void	*_data;		/* Fixed array */
			void	*_qhead;	/* Queue head */
			void	*_qtail;	/* Queue tail */
		};
		struct tt_list	_head;		/* Dynamic list */
	};
	int	(*_enque)(struct tt_queue *queue, const void *e);
	int	(*_deque)(struct tt_queue *queue, void *e);
};

int tt_queue_init(struct tt_queue *queue);
void tt_queue_free(struct tt_queue *queue);

static inline int tt_queue_enque(struct tt_queue *queue, const void *e)
{
	return queue->_enque(queue, e);
}

static inline int tt_queue_deque(struct tt_queue *queue, void *e)
{
	return queue->_deque(queue, e);
}
