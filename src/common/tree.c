/* tree.c
 *
 * Copyright (C) 2014 Yibo Cai
 */
#include <tt.h>
#include <string.h>
#include <tree.h>
#include <_common.h>

/* Red-Black tree operations */
static struct tt_bintree_node *tt_rbtree_searchv(
		const struct tt_bintree *tree, const void *v)
{
	struct tt_bintree_node *node = tree->_root;
	while (node) {
		int c = tree->num.cmp(&tree->num, v, node->data);
		if (c == 0)
			break;
		else if (c < 0)
			node = node->left;
		else
			node = node->right;
	}

	return node;
}

static int tt_rbtree_insertv(struct tt_bintree *tree, const void *v)
{
	struct tt_bintree_node *new = calloc(1, sizeof(struct tt_bintree_node));
	if (!new)
		return -ENOMEM;
	new->data = (void *)v;

	struct tt_bintree_node *parent = NULL, *pos = tree->_root;
	while (pos) {
		parent = pos;
		if (tree->num.cmp(&tree->num, new->data, pos->data) < 0)
			pos = pos->left;
		else
			pos = pos->right;
	}
	new->parent = parent;
	if (!parent)
		tree->_root = new;	/* Empty tree */
	else if (tree->num.cmp(&tree->num, new->data, parent->data) < 0)
		parent->left = new;
	else
		parent->right = new;

	tree->_count++;
	return 0;
}

static int tt_rbtree_deleten(struct tt_bintree *tree, struct tt_bintree_node *v)
{
	/* TODO */
	return 0;
}

/* General in-order walk */
static int tt_bintree_walk_in(const struct tt_bintree_node *node,
		void *context, bool (*walk)(const void *v, void *context))
{
	while (node) {
		if (tt_bintree_walk_in(node->left, context, walk))
			return -ESTOP;
		if (!walk(node->data, context))
			return -ESTOP;
		node = node->right;
	}

	return 0;
}

static struct tt_bintree_ops tt_rbtree_ops = {
	.searchv	= tt_rbtree_searchv,
	.insertv	= tt_rbtree_insertv,
	.deleten	= tt_rbtree_deleten,
	.walk		= tt_bintree_walk_in,
};

void tt_bintree_init(struct tt_bintree *tree)
{
	if (!tree->ops) {
		tt_debug("Red-Black tree initialized");
		tree->ops = &tt_rbtree_ops;
	} else {
		tt_debug("Binary tree initialized");
	}

	_tt_num_select(&tree->num);

	tree->_root = NULL;
	tree->_count = 0;
}

static void tt_bintree_free_internal(struct tt_bintree_node *node)
{
	if (!node)
		return;
	tt_bintree_free_internal(node->left);
	tt_bintree_free_internal(node->right);
	free(node);
}

void tt_bintree_free(struct tt_bintree *tree)
{
	tt_bintree_free_internal(tree->_root);
	tree->_root = NULL;
	tree->_count = 0;
}
