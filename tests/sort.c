/* Test sort algorithms
 *
 * Copyright (C) 2014 Yibo Cai
 */
#include <tt.h>

#include <sort.h>

int data[] = { 254, 1, 60, 2, -1, 6, 8, -10, 2, 5, 7 };

int main(void)
{
	struct tt_sort_input input = {
		.data	= data,
		.count	= ARRAY_SIZE(data),
		.size	= sizeof(data[0]),
		.type	= TT_NUM_SIGNED,
		.alg	= TT_SORT_INSERT,
		.cmp	= NULL,
		.swap	= NULL,
	};

	tt_sort(&input);

	for (int i = 0; i < ARRAY_SIZE(data); i++)
		printf("%d ", data[i]);
	printf("\n");

	return 0;
}
