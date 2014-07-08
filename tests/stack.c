/* Test stack
 *
 * Copyright (C) 2014 Yibo Cai
 */
#include <tt.h>
#include <stack.h>

#define LEN	16

static void test_stack(struct tt_stack *stack)
{
	for (int i = 0; i < LEN + 1; i++) {
		int t = i + 1;
		tt_stack_push(stack, &t);
	}
	for (int i = 0; i < LEN + 1; i++) {
		int t;
		tt_stack_pop(stack, &t);
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

	return 0;
}
