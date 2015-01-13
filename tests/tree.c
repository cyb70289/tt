/* Test tree data structure
 *
 * Copyright (C) 2014 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/tree.h>

#include <string.h>

#define NUM	1024

int keycmp(const struct tt_num *num, const void *v1, const void *v2)
{
	int diff = strlen((const char *)v1) - strlen((const char *)v2);
	return diff ? diff : strcmp((const char *)v1, (const char *)v2);
}

/* a - z, aa - az, ba - bz, ... */
char *v2key(int n)
{
	int i = 0;
	char tmp[10];

	while (n >= 26) {
		tmp[i++] = n % 26 + 'a';
		n /= 26;
	}
	tmp[i++] = n + 'a';

	char *s = malloc(i + 1);
	char *sp = s;
	while (i)
		*sp++ = tmp[--i];
	*sp = '\0';

	return s;
}

int key2v(const char *key)
{
	int n = 0;
	while (*key) {
		n *= 26;
		n += *key - 'a';
		key++;
	}

	return n;
}

static bool walk(const void *key, const void *v, void *context)
{
#if 0
	printf("%s -> %d\n", (const char *)key, *(int *)v);
#endif
	/* Verify key and value pair */
	if (*(int *)v != key2v((const char *)key)) {
		printf("Error in walking tree\n");
		return false;	/* Stop */
	}
	return true;	/* Continue */
}

int tree_height(const struct tt_bintree_node *node)
{
	if (!node)
		return 0;
	return tt_max(tree_height(node->left), tree_height(node->right)) + 1;
}

int main(void)
{
	char *key[NUM];
	int v[NUM];

	struct tt_bintree bt = {
		.knum	= {
			.type	= TT_NUM_USER,
			.size	= 0,	/* Not used */
			.cmp	= keycmp,
			.swap	= NULL,	/* Not used */
		},
		.ops	= NULL,
	};

	tt_bintree_init(&bt);
	printf("\n");

	printf("Insert... ");
	for (int i = 0; i < NUM; i++) {
		key[i] = v2key(i);
		v[i] = i;
		tt_bintree_insert(&bt, key[i], &v[i]);
	}
	printf("Done\n");
	printf("Nodes = %d, height = %d\n", bt._count, tree_height(bt._root));
	printf("\n");

	printf("Walk... ");
	tt_bintree_walk(&bt, NULL, walk);
	printf("Done\n");
	printf("\n");

	struct tt_bintree_node *node = tt_bintree_min(&bt);
	printf("Min: %s -> %d\n", (const char *)node->key, *(int *)node->data);
	node = tt_bintree_max(&bt);
	printf("Max: %s -> %d\n", (const char *)node->key, *(int *)node->data);
	printf("\n");

	printf("Search... ");
	for (int i = 0; i < NUM; i++) {
		node = tt_bintree_search(&bt, key[i]);
		if (!node) {
			printf("Search error\n");
			break;
		}
	}
	printf("Done\n");
	printf("\n");

	printf("Delete... ");
	/* Delete half nodes */
	for (int i = 0; i < NUM; i += 2) {
		node = tt_bintree_search(&bt, key[i]);
		tt_bintree_delete(&bt, node);
		/* Verify deleted */
		if (tt_bintree_search(&bt, key[i])) {
			printf("error\n");
			break;
		}
	}
	/* Verify undeleted */
	for (int i = 1; i < NUM; i += 2) {
		node = tt_bintree_search(&bt, key[i]);
		if (!node) {
			printf("error\n");
			break;
		}
	}
	/* Delete all */
	for (int i = 1; i < NUM; i += 2)
		tt_bintree_delete(&bt, tt_bintree_max(&bt));
	/* Insert & delete again */
	for (int i = 0; i < NUM; i++)
		tt_bintree_insert(&bt, key[i], &v[i]);
	for (int i = 0; i < NUM; i++)
		tt_bintree_delete(&bt, tt_bintree_min(&bt));
	printf("Done\n");
	printf("\n");

	tt_bintree_free(&bt);

	for (int i = 0; i < NUM; i++)
		free(key[i]);

	return 0;
}
