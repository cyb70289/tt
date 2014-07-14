/* Test tree data structure
 *
 * Copyright (C) 2014 Yibo Cai
 */
#include <tt.h>
#include <string.h>
#include <tree.h>

bool walk(const void *v, void *context)
{
	printf("%d ", *(int *)v);
	return true;
}

int main(void)
{
	int s[] = { 5, 4, 6, 2, 7, 1, 8, 3 };

	struct tt_bintree bt = {
		.num	= {
			.type	= TT_NUM_SIGNED,
			.size	= sizeof(int),
			.cmp	= NULL,
			.swap	= NULL,
		},
		.ops	= NULL,
	};

	tt_bintree_init(&bt);
	for (int i = 0; i < ARRAY_SIZE(s); i++)
		tt_bintree_insert(&bt, &s[i]);

	tt_bintree_walk(&bt, NULL, walk);
	printf("\n\n");

	tt_bintree_free(&bt);

	return 0;
}
