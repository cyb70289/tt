/* Tree
 *
 * Copyright (C) 2014 Yibo Cai
 */
#pragma once

#include "_key.h"

enum tt_bintree_color {
	TT_BINTREE_RED,
	TT_BINTREE_BLACK,
};

/* Binary tree */
struct tt_bintree;

/* Tree operations */
struct tt_bintree_ops {
	struct tt_bintree_node* (*min)(struct tt_bintree_node *node);
	struct tt_bintree_node* (*max)(struct tt_bintree_node *node);
	struct tt_bintree_node* (*search)(const struct tt_bintree *tree,
					const void *key);
	int (*insert)(struct tt_bintree *tree, const void *key, const void *v);
	int (*delete)(struct tt_bintree *tree, struct tt_bintree_node *node);
	int (*walk)(const struct tt_bintree_node *node, void *context,
			bool (*walk)(const void *key, const void *v,
				void *context));
};

/* Extended information of a tree node */
union tt_bintree_ext {
	enum tt_bintree_color color;
};

/* Binary tree */
struct tt_bintree {
	struct tt_key knum;	/* data type of "key" */
	struct tt_bintree_ops *ops;

	struct tt_bintree_node *_root;
	uint _count;		/* Node count */
};

/* Tree node */
struct tt_bintree_node {
	void *key;
	void *data;
	struct tt_bintree_node *parent;
	struct tt_bintree_node *left;
	struct tt_bintree_node *right;
	union tt_bintree_ext ext;
};

void tt_bintree_init(struct tt_bintree *tree);
void tt_bintree_free(struct tt_bintree *tree);

static inline struct tt_bintree_node *tt_bintree_min(
		const struct tt_bintree *tree)
{
	return tree->ops->min(tree->_root);
}

static inline struct tt_bintree_node *tt_bintree_max(
		const struct tt_bintree *tree)
{
	return tree->ops->max(tree->_root);
}

static inline struct tt_bintree_node *tt_bintree_search(
		const struct tt_bintree *tree, const void *key)
{
	return tree->ops->search(tree, key);
}

static inline int tt_bintree_insert(struct tt_bintree *tree,
		const void *key, const void *v)
{
	return tree->ops->insert(tree, key, v);
}

static inline int tt_bintree_delete(struct tt_bintree *tree,
		struct tt_bintree_node *node)
{
	return tree->ops->delete(tree, node);
}

static inline void tt_bintree_walk(const struct tt_bintree *tree, void *context,
		bool (*walk)(const void *key, const void *v, void *context))
{
	tree->ops->walk(tree->_root, context, walk);
}
