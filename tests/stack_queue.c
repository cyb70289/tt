/* Test stack and queue
 *
 * Copyright (C) 2014 Yibo Cai
 */
#include <tt.h>
#include <stack.h>
#include <queue.h>

#define LEN	16

static void test_stack(struct tt_stack *stack)
{
	for (int i = 0; i < LEN + 1; i++) {
		int t = i + 1;
		tt_stack_push(stack, &t);
	}
	for (int i = 0; i < LEN + 1; i++) {
		int t;
		if (tt_stack_pop(stack, &t) == 0)
			printf("%d ", t);
	}
	printf("\n");
}

static void test_queue(struct tt_queue *queue)
{
	int t;
	for (int i = 0; i < LEN + 1; i++) {
		t = i + 1;
		tt_queue_enque(queue, &t);
	}
	for (int i = 0; i < LEN - 1; i++)
		tt_queue_deque(queue, &t);
	for (int i = LEN - 2; i >= 0; i--) {
		t = i + 1;
		tt_queue_enque(queue, &t);
	}

	for (int i = 0; i < LEN + 1; i++) {
		if (tt_queue_deque(queue, &t) == 0)
			printf("%d ", t);
	}
	printf("\n");
}

int main(void)
{
	struct tt_stack stack = {
		.cap	= LEN,
		.size	= sizeof(int),
	};

	printf("Testing static stack...\n");
	tt_stack_init(&stack);
	test_stack(&stack);
	tt_stack_free(&stack);
	printf("\n");

	printf("Testing dynamic stack...\n");
	stack.cap = 0;
	tt_stack_init(&stack);
	test_stack(&stack);
	tt_stack_free(&stack);
	printf("\n");

	struct tt_queue queue = {
		.cap	= LEN,
		.size	= sizeof(int),
	};

	printf("Testing static queue...\n");
	tt_queue_init(&queue);
	test_queue(&queue);
	tt_queue_free(&queue);
	printf("\n");

	printf("Testing dynamic queue...\n");
	queue.cap = 0;
	tt_queue_init(&queue);
	test_queue(&queue);
	tt_queue_free(&queue);
	printf("\n");

	return 0;
}
