/* General stack
 *
 * Copyright (C) 2014 Yibo Cai
 *
 * XXX: Stack elements only ensure 8-bytes alignment
 */
#pragma once

#include <list.h>

struct tt_stack {
	uint	cap;	/* Maxiam elements, 0 - dynamic */
	uint	size;	/* Byte size of each element */
	uint	_count;	/* Elements count */
	union {
		struct {
			void	*_data;	/* Fixed array */
			void	*_top;	/* Stack top pointer */
		};
		struct tt_list	_head;	/* Dynamic list */
	};
	int	(*_push)(struct tt_stack *stack, const void *e);
	int	(*_pop)(struct tt_stack *stack, void *e);
};

int tt_stack_init(struct tt_stack *stack);
void tt_stack_free(struct tt_stack *stack);

static inline int tt_stack_push(struct tt_stack *stack, const void *e)
{
	return stack->_push(stack, e);
}
static inline int tt_stack_pop(struct tt_stack *stack, void *e)
{
	return stack->_pop(stack, e);
}
