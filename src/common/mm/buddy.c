/* !!DEPRECATED!! It's way slower than malloc.
 *
 * Random size buffer management - Buddy algorithm
 *
 * Copyright (C) 2015 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/common/list.h>
#include "../lib.h"

#include <pthread.h>

/* Memory pool
 * - Block size = 2^n - 8
 * - Layout
 * Block list         Block-1                    Block-2
 * +----------+       +------+-----------+       +------+-----------+
 * | heads[0] | <===> | link |  56 bytes | <===> | link |  56 bytes | <===> ...
 * +----------+       +------+-----------+       +------+-----------+
 * | heads[1] | <===> | link | 120 bytes | <===> | link | 120 bytes | <===> ...
 * +----------+       +------+-----------+       +------+-----------+
 * | heads[2] | <===> | link | 248 bytes | <===> | link | 248 bytes | <===> ...
 * +----------+       +------+-----------+       +------+-----------+
 * | heads[3] | <===> | link | 504 bytes | <===> | link | 504 bytes | <===> ...
 * +----------+       +------+-----------+       +------+-----------+
 * |   ...    | <===> | ...  |    ...    | <===> | ...  |    ...    | <===> ...
 * +----------+       +------+-----------+       +------+-----------+
 */

#if (CONFIG_POOL_SIZE < 4096 || CONFIG_POOL_SIZE > 1 << 31)
#error "Pool size error"
#endif

#define BLOCK_GRAN_ORDER	6	/* Granuality (1 << 6)*/
#define BLOCK_HEAD_SIZE		8	/* Block header size */
#define BLOCK_MAGIC_FREE	0xBABE
#define BLOCK_MAGIC_ALLOC	0xFACE

struct pool {
	uint size;		/* Buffer size */
	uint free;		/* Free buffer size */
	uint missed;		/* Failed allocations */
	const void *buf;	/* Buffer address */
	int blks;		/* Block header array size */
	struct tt_list *heads;	/* Block header array */
	pthread_mutex_t mtx;
};

struct pool_block {
	ushort magic;		/* Guard */
	short blk_idx;		/* Block size index */
	short prev_blk_idx;	/* Previous mergeable block size */
	short next_blk_idx;	/* Next mergeable block size */
	union {
		void *ptr[0];		/* Data buffer pointer */
		struct tt_list link;	/* Link to next block */
	};
};

static struct pool __pool = {
	.mtx = PTHREAD_MUTEX_INITIALIZER,
};

static const uint msb_to_int[] = {
	BIT(0),  BIT(1),  BIT(2),  BIT(3),  BIT(4),  BIT(5),  BIT(6),  BIT(7),
	BIT(8),  BIT(9),  BIT(10), BIT(11), BIT(12), BIT(13), BIT(14), BIT(15),
	BIT(16), BIT(17), BIT(18), BIT(19), BIT(20), BIT(21), BIT(22), BIT(23),
	BIT(24), BIT(25), BIT(26), BIT(27), BIT(28), BIT(29), BIT(30), BIT(31),
};

/* Get MSB. i != 0. */
static inline uint __get_msb(uint i)
{
	tt_assert_fa(i);
	return 31 - __builtin_clz(i);
}

/* Round down to 2^n. i != 0. */
static inline uint __round_down_2n(uint i)
{
	return msb_to_int[__get_msb(i)];
}

/* Get MSB after rounded up to 2^n. i <= 1<<31. */
static int __round_up_2n_msb(uint i)
{
	int msb = __get_msb(i);
	if (__builtin_popcount(i) == 1) {
		return msb;
	} else {
		tt_assert_fa(i < BIT(31));
		return msb + 1;
	}
}

static inline int pool_size_to_index(uint size)
{
	int idx = __round_up_2n_msb(size + BLOCK_HEAD_SIZE) - BLOCK_GRAN_ORDER;
	if (idx < 0)
		idx = 0;
	return idx;
}

static inline uint pool_index_to_size(int index)
{
	return msb_to_int[index + BLOCK_GRAN_ORDER];
}

static __attribute__ ((constructor)) void pool_init(void)
{
	if (offset_of(struct pool_block, ptr) != BLOCK_HEAD_SIZE) {
		tt_error("Invalid pool_block header size");
		abort();
	}

	struct pool *pool = &__pool;

	/* Round pool size down to 2^n */
	pool->size = pool->free = __round_down_2n(CONFIG_POOL_SIZE);
	if (pool->size != CONFIG_POOL_SIZE)
		tt_debug("Pool size truncated: %u -> %u",
				CONFIG_POOL_SIZE, pool->size);

	/* Maximum block size = pool->size - BLOCK_HEAD_SIZE */
	pool->blks = pool_size_to_index(pool->size - BLOCK_HEAD_SIZE) + 1;

	/* Allocate block buffer */
	pool->buf = malloc(pool->size);
	if (!pool->buf) {
		tt_error("Pool allocation failed!");
		abort();
		return;
	}

	/* Initialize block list */
	pool->heads = malloc(pool->blks * sizeof(struct tt_list));
	for (int i = 0; i < pool->blks; i++)
		tt_list_init(&pool->heads[i]);

	/* Pre-allocate one large block */
	struct pool_block *blk = (struct pool_block *)pool->buf;
	blk->magic = BLOCK_MAGIC_FREE;
	blk->blk_idx = pool->blks - 1;
	blk->prev_blk_idx = -1;
	blk->next_blk_idx = -1;
	tt_list_add(&blk->link, &pool->heads[blk->blk_idx]);

	tt_info("Pool created: %u", pool->size);
}

void *_tt_get_buf(size_t sz)
{
	struct pool *pool = &__pool;

	pthread_mutex_lock(&pool->mtx);

	/* Search for free buffer */
	int idx, blk_idx = pool_size_to_index(sz);
	for (idx = blk_idx; idx < pool->blks; idx++)
		if (!tt_list_isempty(&pool->heads[idx]))
			break;

	/* Fallback to malloc if no free buffer available */
	if (idx == pool->blks) {
		pool->missed++;
		pthread_mutex_unlock(&pool->mtx);
		tt_debug("Cannot get buffer: %u %u", sz, pool->free);
		return malloc(sz);
	}

	/* Get buffer, remove from free list */
	struct pool_block *blk = container_of(pool->heads[idx].next,
			struct pool_block, link);
	tt_assert_fa(blk->magic == BLOCK_MAGIC_FREE);
	blk->magic = BLOCK_MAGIC_ALLOC;
	blk->blk_idx = blk_idx;
	if (idx != blk_idx)
		blk->next_blk_idx = blk_idx;
	tt_list_del(&blk->link);
	pool->free -= pool_index_to_size(blk_idx);

	/* Link residual blocks to related free list */
	struct pool_block *newblk;
	void *ptr = (void *)blk + pool_index_to_size(blk_idx);
	for (int i = blk_idx; i < idx; i++) {
		newblk = ptr;
		newblk->magic = BLOCK_MAGIC_FREE;
		newblk->blk_idx = newblk->prev_blk_idx = i;
		newblk->next_blk_idx = -1;
		tt_list_add(&newblk->link, &pool->heads[i]);
		ptr += pool_index_to_size(i);
	}

	pthread_mutex_unlock(&pool->mtx);

	return blk->ptr;
}

void _tt_put_buf(void *buf)
{
	struct pool *pool = &__pool;

	/* May allocated by malloc */
	if (buf < pool->buf || buf >= pool->buf + pool->size) {
		free(buf);
		return;
	}

	struct pool_block *blk = buf - BLOCK_HEAD_SIZE;
	tt_assert_fa(blk->magic == BLOCK_MAGIC_ALLOC);

	pthread_mutex_lock(&pool->mtx);

	blk->magic = BLOCK_MAGIC_FREE;
	blk->link.next = NULL;	/* Not in free list */

	/* Combine with adjacent blocks */
	while (1) {
		/* Merge following free blocks */
		while (blk->next_blk_idx >= 0) {
			tt_assert_fa(blk->blk_idx == blk->next_blk_idx);
			tt_assert_fa(blk->next_blk_idx < blk->prev_blk_idx ||
					blk->prev_blk_idx < 0);

			/* Check next block */
			struct pool_block *blkn =
				(void *)blk + pool_index_to_size(blk->blk_idx);
			tt_assert_fa((void *)blkn < pool->buf + pool->size);
			if (blkn->magic == BLOCK_MAGIC_ALLOC)
				break;
			tt_assert_fa(blkn->magic == BLOCK_MAGIC_FREE);

			/* If next block is of same size, we can merge */
			if (blkn->blk_idx < blk->next_blk_idx)
				break;
			tt_assert_fa(blkn->blk_idx == blk->next_blk_idx);

			/* Double block size */
			blk->blk_idx++;
			blk->next_blk_idx++;
			/* Check if block is fully merged */
			if (blk->prev_blk_idx < 0) {
				/* Current block is at pool beginning */
				if (blk->blk_idx == pool->blks - 1)
					blk->next_blk_idx = -1;
			} else {
				if (blk->next_blk_idx == blk->prev_blk_idx)
					blk->next_blk_idx = -1;
			}

			/* Remove next block from free list */
			if (blkn->link.next) {
				tt_list_del(&blkn->link);
				pool->free -= pool_index_to_size(blkn->blk_idx);
			}
		}

		/* Current block is at pool beginning. Or not fully merged. */
		if (blk->prev_blk_idx < 0 || blk->next_blk_idx >= 0)
			break;

		tt_assert_fa(blk->prev_blk_idx == blk->blk_idx);

		/* Check previous block */
		struct pool_block *blkp =
			(void *)blk - pool_index_to_size(blk->prev_blk_idx);
		tt_assert_fa((void *)blkp >= pool->buf);
		if (blkp->magic == BLOCK_MAGIC_ALLOC)
			break;
		tt_assert_fa(blkp->magic == BLOCK_MAGIC_FREE);

		/* Check if there're gaps between previous and current block */
		if (blkp->next_blk_idx != blk->blk_idx)
			break;

		/* Move to previous block, merge with next(current) block */
		blk = blkp;
		tt_list_del(&blk->link);
		pool->free -= pool_index_to_size(blk->blk_idx);
	}

	/* Insert to free list */
	tt_list_add(&blk->link, &pool->heads[blk->blk_idx]);
	pool->free += pool_index_to_size(blk->blk_idx);

	pthread_mutex_unlock(&pool->mtx);
}

/* Only for testing purpose */
const struct pool *__get_pool(void)
{
	return &__pool;
}
