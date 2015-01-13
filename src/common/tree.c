/* tree.c
 *
 * Copyright (C) 2014 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/tree.h>
#include "_common.h"

#include <string.h>

/* Red-Black tree operations */
static inline struct tt_bintree_node *tt_rbtree_min(
		struct tt_bintree_node *node)
{
	if (node) {
		while (node->left)
			node = node->left;
	}
	return node;
}

static inline struct tt_bintree_node *tt_rbtree_max(
		struct tt_bintree_node *node)
{
	if (node) {
		while (node->right)
			node = node->right;
	}
	return node;
}

static struct tt_bintree_node *tt_rbtree_search(
		const struct tt_bintree *tree, const void *key)
{
	struct tt_bintree_node *node = tree->_root;
	while (node) {
		int c = tree->knum.cmp(&tree->knum, key, node->key);
		if (c == 0)
			break;
		else if (c < 0)
			node = node->left;
		else
			node = node->right;
	}
	return node;
}

/*       y      left rotate      x
 *      / \    <------------    / \
 *     x   c                   a   y
 *    / \      ------------>      / \
 *   a   b      right rotate     b   c
 */
static void tt_rbtree_rotate_left(struct tt_bintree *tree,
		struct tt_bintree_node *x)
{
	struct tt_bintree_node *y = x->right;
	x->right = y->left;
	if (y->left)
		y->left->parent = x;
	y->parent = x->parent;
	if (!x->parent)
		tree->_root = y;
	else if (x == x->parent->left)
		x->parent->left = y;
	else
		x->parent->right = y;
	y->left = x;
	x->parent = y;
}

static void tt_rbtree_rotate_right(struct tt_bintree *tree,
		struct tt_bintree_node *y)
{
	struct tt_bintree_node *x = y->left;
	y->left = x->right;
	if (x->right)
		x->right->parent = y;
	x->parent = y->parent;
	if (!y->parent)
		tree->_root = x;
	else if (y == y->parent->left)
		y->parent->left = x;
	else
		y->parent->right = x;
	x->right = y;
	y->parent = x;
}

static int tt_rbtree_insert(struct tt_bintree *tree, const void *key,
		const void *v)
{
	struct tt_bintree_node *z = calloc(1, sizeof(struct tt_bintree_node));
	if (!z)
		return -TT_ENOMEM;
	z->key = (void *)key;
	z->data = (void *)v;
	z->ext.color = TT_COLOR_RED;

	/* Insert as normal binary search tree */
	struct tt_bintree_node *y = NULL, *x = tree->_root;
	while (x) {
		y = x;
		if (tree->knum.cmp(&tree->knum, z->key, x->key) < 0)
			x = x->left;
		else
			x = x->right;
	}
	z->parent = y;
	if (!y)
		tree->_root = z;	/* Empty tree */
	else if (tree->knum.cmp(&tree->knum, z->key, y->key) < 0)
		y->left = z;
	else
		y->right = z;

	/* Rotate to maintain red-black property */
	while (z->parent && z->parent->ext.color == TT_COLOR_RED) {
		if (z->parent == z->parent->parent->left) {
			y = z->parent->parent->right;
			if (y && y->ext.color == TT_COLOR_RED) {
				z->parent->ext.color = TT_COLOR_BLACK;
				y->ext.color = TT_COLOR_BLACK;
				z->parent->parent->ext.color = TT_COLOR_RED;
				z = z->parent->parent;
			} else if (z == z->parent->right) {
				z = z->parent;
				tt_rbtree_rotate_left(tree, z);
			} else {
				z->parent->ext.color = TT_COLOR_BLACK;
				z->parent->parent->ext.color = TT_COLOR_RED;
				tt_rbtree_rotate_right(tree, z->parent->parent);
			}
		} else {
			y = z->parent->parent->left;
			if (y && y->ext.color == TT_COLOR_RED) {
				z->parent->ext.color = TT_COLOR_BLACK;
				y->ext.color = TT_COLOR_BLACK;
				z->parent->parent->ext.color = TT_COLOR_RED;
				z = z->parent->parent;
			} else if (z == z->parent->left) {
				z = z->parent;
				tt_rbtree_rotate_right(tree, z);
			} else {
				z->parent->ext.color = TT_COLOR_BLACK;
				z->parent->parent->ext.color = TT_COLOR_RED;
				tt_rbtree_rotate_left(tree, z->parent->parent);
			}
		}
	}
	tree->_root->ext.color = TT_COLOR_BLACK;

	tree->_count++;
	return 0;
}

/* Transplant for deletion */
static void tt_rbtree_trans(struct tt_bintree *tree,
		struct tt_bintree_node *u, struct tt_bintree_node *v)
{
	if (!u->parent)
		tree->_root = v;
	else if (u == u->parent->left)
		u->parent->left = v;
	else
		u->parent->right = v;
	if (v)
		v->parent = u->parent;
}

static int tt_rbtree_delete(struct tt_bintree *tree, struct tt_bintree_node *z)
{
	struct tt_bintree_node *w, *x, *y = z;
	enum tt_color ycolor = y->ext.color;

	/* Delete node */
	if (!z->left) {
		x = z->right;
		tt_rbtree_trans(tree, z, z->right);
	} else if (!z->right) {
		x = z->left;
		tt_rbtree_trans(tree, z, z->left);
	} else {
		y = tt_rbtree_min(z->right);
		ycolor = y->ext.color;
		x = y->right;
		if (y->parent == z) {
			if (x)
				x->parent = y;
		} else {
			tt_rbtree_trans(tree, y, y->right);
			y->right = z->right;
			y->right->parent = y;
		}
		tt_rbtree_trans(tree, z, y);
		y->left = z->left;
		y->left->parent = y;
		y->ext.color = z->ext.color;
	}
	free(z);

	if (ycolor != TT_COLOR_BLACK)
		return 0;

	/* Rotate to maintain red-black property */
	while (x && x != tree->_root && x->ext.color == TT_COLOR_BLACK) {
		if (x == x->parent->left) {
			w = x->parent->right;
			if (w->ext.color == TT_COLOR_RED) {
				w->ext.color = TT_COLOR_BLACK;
				x->parent->ext.color = TT_COLOR_RED;
				tt_rbtree_rotate_left(tree, x->parent);
				w = x->parent->right;
			}
			bool l = !w->left ||
				w->left->ext.color == TT_COLOR_BLACK;
			bool r = !w->right ||
				w->right->ext.color == TT_COLOR_BLACK;
			if (l && r) {
				w->ext.color = TT_COLOR_RED;
				x = x->parent;
			} else if (r) {
				w->left->ext.color = TT_COLOR_BLACK;
				w->ext.color = TT_COLOR_RED;
				tt_rbtree_rotate_right(tree, w);
				w = x->parent->right;
			} else {
				w->ext.color = x->parent->ext.color;
				x->parent->ext.color = TT_COLOR_BLACK;
				w->right->ext.color = TT_COLOR_BLACK;
				tt_rbtree_rotate_left(tree, x->parent);
				x = tree->_root;
			}
		} else {
			w = x->parent->left;
			if (w->ext.color == TT_COLOR_RED) {
				w->ext.color = TT_COLOR_BLACK;
				x->parent->ext.color = TT_COLOR_RED;
				tt_rbtree_rotate_right(tree, x->parent);
				w = x->parent->left;
			}
			bool r = !w->right ||
				w->right->ext.color == TT_COLOR_BLACK;
			bool l = !w->left ||
				w->left->ext.color == TT_COLOR_BLACK;
			if (r && l) {
				w->ext.color = TT_COLOR_RED;
				x = x->parent;
			} else if (l) {
				w->right->ext.color = TT_COLOR_BLACK;
				w->ext.color = TT_COLOR_RED;
				tt_rbtree_rotate_left(tree, w);
				w = x->parent->left;
			} else {
				w->ext.color = x->parent->ext.color;
				x->parent->ext.color = TT_COLOR_BLACK;
				w->left->ext.color = TT_COLOR_BLACK;
				tt_rbtree_rotate_right(tree, x->parent);
				x = tree->_root;
			}
		}
	}
	if (x)
		x->ext.color = TT_COLOR_BLACK;

	return 0;
}

/* General in-order walk */
static int tt_rbtree_walk_in(const struct tt_bintree_node *node, void *context,
		bool (*walk)(const void *key, const void *v, void *context))
{
	while (node) {
		if (tt_rbtree_walk_in(node->left, context, walk))
			return -TT_ESTOP;
		if (!walk(node->key, node->data, context))
			return -TT_ESTOP;
		node = node->right;
	}

	return 0;
}

static struct tt_bintree_ops tt_rbtree_ops = {
	.min	= tt_rbtree_min,
	.max	= tt_rbtree_max,
	.search	= tt_rbtree_search,
	.insert	= tt_rbtree_insert,
	.delete	= tt_rbtree_delete,
	.walk	= tt_rbtree_walk_in,
};

/* A dummy swap routine to satisfy _tt_num_select() */
static void tt_bintree_swap_dummy(const struct tt_num *num, void *v1, void *v2)
{
	tt_info("No swap routine");
}

void tt_bintree_init(struct tt_bintree *tree)
{
	if (!tree->ops) {
		tt_debug("Red-Black tree initialized");
		tree->ops = &tt_rbtree_ops;
	} else {
		tt_debug("Binary tree initialized");
	}

	if (!tree->knum.swap)
		tree->knum.swap = tt_bintree_swap_dummy;
	_tt_num_select(&tree->knum);

	tree->_root = NULL;
	tree->_count = 0;
}

static void tt_bintree_free_internal(struct tt_bintree_node *node)
{
	if (node) {
		tt_bintree_free_internal(node->left);
		tt_bintree_free_internal(node->right);
		free(node);
	}
}

void tt_bintree_free(struct tt_bintree *tree)
{
	tt_bintree_free_internal(tree->_root);
	tree->_root = NULL;
	tree->_count = 0;
}
