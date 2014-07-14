/* tree.h
 *
 * Copyright (C) 2014 Yibo Cai
 */
#pragma once

/* Binary tree */
struct tt_bintree;

struct tt_bintree_ops {
	struct tt_bintree_node*
		(*searchv)(const struct tt_bintree *tree, const void *v);
	int	(*insertv)(struct tt_bintree *tree, const void *v);
	int	(*deleten)(struct tt_bintree *tree,
			struct tt_bintree_node *node);
	int	(*walk)(const struct tt_bintree_node *node, void *context,
			bool (*walk)(const void *v, void *context));
};

struct tt_bintree {
	struct tt_num		num;
	struct tt_bintree_ops	*ops;

	struct tt_bintree_node	*_root;
	uint	_count;	/* Node count */
};

struct tt_bintree_node {
	struct tt_bintree_node	*parent;
	void	*data;
	struct tt_bintree_node	*left;
	struct tt_bintree_node	*right;
};

void tt_bintree_init(struct tt_bintree *tree);
void tt_bintree_free(struct tt_bintree *tree);

static inline struct tt_bintree_node *tt_bintree_search(
		const struct tt_bintree *tree, const void *v)
{
	return tree->ops->searchv(tree, v);
}

static inline int tt_bintree_insert(struct tt_bintree *tree, const void *v)
{
	return tree->ops->insertv(tree, v);
}

static inline int tt_bintree_delete(struct tt_bintree *tree,
		struct tt_bintree_node *node)
{
	return tree->ops->deleten(tree, node);
}

static inline void tt_bintree_walk(const struct tt_bintree *tree, void *context,
		bool (*walk)(const void *v, void *context))
{
	tree->ops->walk(tree->_root, context, walk);
}
