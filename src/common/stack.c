/* General stack implementation
 *
 * Copyright (C) 2014 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/common/stack.h>

#include <string.h>

/* Static allocated stack: Empty, Incremental */
static int tt_stack_push_array(struct tt_stack *stack, const void *e)
{
	if (stack->_count >= stack->cap) {
		tt_debug("Overflow");
		return TT_EOVERFLOW;
	}
	memcpy(stack->_top, e, stack->esize);
	stack->_top += stack->esize;

	stack->_count++;
	return 0;
}

static int tt_stack_pop_array(struct tt_stack *stack, void *e)
{
	if (stack->_count == 0) {
		tt_debug("Underflow");
		return TT_EUNDERFLOW;
	}
	stack->_top -= stack->esize;
	memcpy(e, stack->_top, stack->esize);

	stack->_count--;
	return 0;
}

/* Dynamic linked stack */
static int tt_stack_push_list(struct tt_stack *stack, const void *e)
{
	void *se = malloc(sizeof(struct tt_list) + stack->esize);
	if (!se)
		return TT_EOVERFLOW;
	memcpy(se + sizeof(struct tt_list), e, stack->esize);
	tt_list_add(se, &stack->_head);

	stack->_count++;
	return 0;
}

static int tt_stack_pop_list(struct tt_stack *stack, void *e)
{
	if (stack->_count == 0) {
		tt_debug("Underflow");
		return TT_EUNDERFLOW;
	}
	void *top = stack->_head.prev;
	memcpy(e, top + sizeof(struct tt_list), stack->esize);
	tt_list_del(top);
	free(top);

	stack->_count--;
	return 0;
}

int tt_stack_init(struct tt_stack *stack)
{
	if (stack->cap) {
		/* Allocate fixed array */
		stack->_data = malloc(stack->esize * stack->cap);
		if (!stack->_data)
			return TT_ENOMEM;
		stack->_top = stack->_data;
		stack->_push = tt_stack_push_array;
		stack->_pop = tt_stack_pop_array;
		tt_debug("Stack created, max elements = %u", stack->cap);
	} else {
		/* XXX: Allocate tt_list struct before element,
		 * the stack element only ensures 8-bytes alignment
		 */
		tt_assert(sizeof(struct tt_list) % 8 == 0);
		/* Initialize list head */
		tt_list_init(&stack->_head);
		stack->_push = tt_stack_push_list;
		stack->_pop = tt_stack_pop_list;
		tt_debug("Dynamic stack created");
	}

	stack->_count = 0;
	return 0;
}

void tt_stack_free(struct tt_stack *stack)
{
	if (stack->cap) {
		/* Free the array */
		free(stack->_data);
		stack->_data = NULL;
	} else {
		/* Free the list */
		struct tt_list *pos, *tmp;
		tt_list_for_each_safe(pos, tmp, &stack->_head)
			free(pos);
	}

	tt_debug("Stack destroyed");
	stack->_count = 0;
}
