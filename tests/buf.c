/* Buffer management test
 *
 * Copyright (C) 2015 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/common/list.h>
#include <common/lib.h>

#include <string.h>
#include <pthread.h>

#pragma GCC diagnostic ignored "-Wunused-function"

#define BLOCK_GRAN_ORDER	6
#define BLOCK_MAGIC_FREE	0xBABE
#define BLOCK_MAGIC_ALLOC	0xFACE

#if 0
struct pool {
	uint size;
	uint free;
	uint missed;
	const void *buf;
	int blks;
	struct tt_list *heads;
};

struct pool_block {
	ushort magic;
	short blk_idx;
};

/* Check pool sanity. No locking. */
const struct pool *__get_pool(void);
static int __tt_check_buf(const struct pool **ppool)
{
	const struct pool *pool = __get_pool();
	*ppool = pool;

	/* Verify all blocks */
	const void *buf = pool->buf;
	const struct pool_block *blk = buf;

	while (buf < pool->buf + pool->size) {
		if (((size_t)blk & 7)) {
			tt_error("Buffer address alignment error");
			return TT_EINVAL;
		}

		if (blk->magic != BLOCK_MAGIC_FREE &&
				blk->magic != BLOCK_MAGIC_ALLOC) {
			tt_error("Buffer magic error");
			return TT_EINVAL;
		}

		uint blk_sz = 1 << (blk->blk_idx + BLOCK_GRAN_ORDER);
		uint align = buf - pool->buf;
		if ((align % blk_sz)) {
			tt_error("Buffer alignment error");
			return TT_EINVAL;
		}

		buf += blk_sz;
		blk = buf;
	}
	tt_assert(buf == pool->buf + pool->size);

	return 0;
}

static int get_count(struct tt_list *head)
{
	int n = 0;
	struct tt_list *pos;
	tt_list_for_each(pos, head)
		n++;
	return n;
}

/* Allocate/free random size buffer */
static int buddy_alloc_free(void)
{
	const struct pool *pool;
	void *b[1024];

	/* Allocate 1024 random size blocks */
	for (int i = 0; i < 1024; i++) {
		int n = rand() % 20000 + 200;
		b[i] = _tt_get_buf(n);
		memset(b[i], 0xCC, n);
		if (__tt_check_buf(&pool))
			return TT_EINVAL;
	}

	/* Free in pre-order */
	for (int i = 0; i < 1024; i++) {
		_tt_put_buf(b[i]);
		if (__tt_check_buf(&pool))
			return TT_EINVAL;
	}
	for (int i = 0; i < pool->blks - 1; i++)
		if (get_count(&pool->heads[i])) {
			tt_error("Free list should be empty: %d", i);
			return TT_EINVAL;
		}
	if (get_count(&pool->heads[pool->blks-1]) != 1) {
		tt_error("Last free list should be 1");
		return TT_EINVAL;
	}
	if (pool->free != CONFIG_POOL_SIZE) {
		tt_error("Free size error");
		return TT_EINVAL;
	}

	/* Allocate 1024 random size blocks */
	for (int i = 0; i < 1024; i++) {
		int n = rand() % 20000 + 200;
		b[i] = _tt_get_buf(n);
		memset(b[i], 0xCC, n);
		if (__tt_check_buf(&pool))
			return TT_EINVAL;
	}

	/* Free in post-order */
	for (int i = 1023; i >= 0; i--) {
		_tt_put_buf(b[i]);
		if (__tt_check_buf(&pool))
			return TT_EINVAL;
	}
	for (int i = 0; i < pool->blks - 1; i++)
		if (get_count(&pool->heads[i])) {
			tt_error("Free list should be empty: %d", i);
			return TT_EINVAL;
		}
	if (get_count(&pool->heads[pool->blks-1]) != 1) {
		tt_error("Last free list should be 1");
		return TT_EINVAL;
	}
	if (pool->free != CONFIG_POOL_SIZE) {
		tt_error("Free size error");
		return TT_EINVAL;
	}

	/* Allocate 1024 random size blocks */
	for (int i = 0; i < 1024; i++) {
		int n = rand() % 20000 + 200;
		b[i] = _tt_get_buf(n);
		memset(b[i], 0xCC, n);
		if (__tt_check_buf(&pool))
			return TT_EINVAL;
	}

	/* Free in random-order */
	int cnt = 0, freed[1024];
	for (int i = 0; i < 1024; i++)
		freed[i] = 0;
	while (cnt < 1024) {
		int i = rand() % 1024;
		if (freed[i])
			continue;
		freed[i] = 1;
		cnt++;
		_tt_put_buf(b[i]);
		if (__tt_check_buf(&pool))
			return TT_EINVAL;
	}
	for (int i = 0; i < pool->blks - 1; i++)
		if (get_count(&pool->heads[i])) {
			tt_error("Free list should be empty: %d", i);
			return TT_EINVAL;
		}
	if (get_count(&pool->heads[pool->blks-1]) != 1) {
		tt_error("Last free list should be 1");
		return TT_EINVAL;
	}
	if (pool->free != CONFIG_POOL_SIZE) {
		tt_error("Free size error");
		return TT_EINVAL;
	}

	return 0;
}

static void *buddy_thread(void *arg)
{
	size_t n = (size_t)arg;

	while (n--) {
		/* Allocate at most 132000 random size blocks */
		void *b[1024];
		for (int i = 0; i < 1024; i++)
			b[i] = 0;
		int total = 0, count = 0;
		for (int i = 0; i < 1024; i++) {
			int n = rand() % 2000;
			b[i] = _tt_get_buf(n);
			memset(b[i], 0xCC, n);
			count++;
			total += n;
			if (total > 132000)
				break;
		}

		/* Free in random-order */
		int cnt = 0, freed[1024];
		for (int i = 0; i < 1024; i++)
			freed[i] = 0;
		while (cnt < count) {
			int i = rand() % 1024;
			if (i >= count || freed[i])
				continue;
			freed[i] = 1;
			cnt++;
			_tt_put_buf(b[i]);
		}
	}

	return 0;
}

static int buddy_test(int n)
{
	printf("Verify buddy algorithm %d rounds\n", n);

	printf("alloc, free...\n");
	for (int i = 0; i < n; i++) {
		int ret = buddy_alloc_free();
		if (ret)
			return ret;
	}

	printf("concurrency...\n");
	pthread_t t[8];
	for (int i = 0; i < 8; i++)
		pthread_create(&t[i], NULL, buddy_thread, (void *)(size_t)n);
	void *res;
	for (int i = 0; i < 8; i++)
		pthread_join(t[i], &res);
	/* Verify */
	const struct pool *pool;
	if (__tt_check_buf(&pool))
		return TT_EINVAL;
	for (int i = 0; i < pool->blks - 1; i++)
		if (get_count(&pool->heads[i])) {
			tt_error("Free list should be empty: %d", i);
			return TT_EINVAL;
		}
	if (get_count(&pool->heads[pool->blks-1]) != 1) {
		tt_error("Last free list should be 1");
		return TT_EINVAL;
	}
	if (pool->free != CONFIG_POOL_SIZE) {
		tt_error("Free size error");
		return TT_EINVAL;
	}

	return 0;
}
#endif

struct tt_slab {
	uint magic;		/* Slab tag */
	char name[16];		/* Slab name */
	pthread_spinlock_t spl;
	uint missed;		/* Failed allocations */

	void *obj_buf;		/* Object buffer */
	uint buf_sz;		/* Object buffer size = obj_sz * obj_cnt */
	uint obj_sz;		/* Object size */

	uint obj_cnt;		/* Maximum number of objects */
	int obj_free;		/* Free objects */
} *__slab;

static int slab_alloc_free(void)
{
	void *obj[1000];

	/* Allocate all objects */
	for (int i = 0; i < 1000; i++) {
		obj[i] = _tt_slab_get_obj(__slab);
		memset(obj[i], 0xCC, __slab->obj_sz);
	}
	if (__slab->obj_free != 0) {
		tt_error("Free count error");
		return TT_EINVAL;
	}

	/* Allocate/free an extra object out of slab capacity */
	void *v = _tt_slab_get_obj(__slab);
	_tt_slab_put_obj(__slab, v);

	/* Free all objects in pre-order */
	for (int i = 0; i < 1000; i++)
		_tt_slab_put_obj(__slab, obj[i]);
	if (__slab->obj_free != __slab->obj_cnt) {
		tt_error("Free count error");
		return TT_EINVAL;
	}

	/* Allocate all objects */
	for (int i = 0; i < 1000; i++) {
		obj[i] = _tt_slab_get_obj(__slab);
		memset(obj[i], 0xCC, __slab->obj_sz);
	}
	if (__slab->obj_free != 0) {
		tt_error("Free count error");
		return TT_EINVAL;
	}

	/* Free all objects in post-order */
	for (int i = 999; i >= 0; i--)
		_tt_slab_put_obj(__slab, obj[i]);
	if (__slab->obj_free != __slab->obj_cnt) {
		tt_error("Free count error");
		return TT_EINVAL;
	}

	/* Allocate half objects */
	for (int i = 0; i < 500; i++) {
		obj[i] = _tt_slab_get_obj(__slab);
		memset(obj[i], 0xCC, __slab->obj_sz);
	}
	if (__slab->obj_free != __slab->obj_cnt - 500) {
		tt_error("Free count error");
		return TT_EINVAL;
	}

	/* Free in random order */
	int cnt = 0, freed[1000];
	for (int i = 0; i < 1000; i++)
		freed[i] = 0;
	while (cnt < 500) {
		int i = rand() % 500;
		if (freed[i])
			continue;
		freed[i] = 1;
		cnt++;
		_tt_slab_put_obj(__slab, obj[i]);
	}
	if (__slab->obj_free != __slab->obj_cnt) {
		tt_error("Free count error");
		return TT_EINVAL;
	}

	/* Allocate all objects */
	for (int i = 0; i < 1000; i++) {
		obj[i] = _tt_slab_get_obj(__slab);
		memset(obj[i], 0xCC, __slab->obj_sz);
	}
	if (__slab->obj_free != 0) {
		tt_error("Free count error");
		return TT_EINVAL;
	}

	/* Free in random order */
	cnt = 0;
	for (int i = 0; i < 1000; i++)
		freed[i] = 0;
	while (cnt < 1000) {
		int i = rand() % 1000;
		if (freed[i])
			continue;
		freed[i] = 1;
		cnt++;
		_tt_slab_put_obj(__slab, obj[i]);
	}
	if (__slab->obj_free != __slab->obj_cnt) {
		tt_error("Free count error");
		return TT_EINVAL;
	}

	return 0;
}

static void *slab_thread(void *arg)
{
	size_t n = (size_t)arg;

	while (n--) {
		/* Allocate 1200 objects */
		void *b[1200];
		for (int i = 0; i < 1200; i++)
			b[i] = _tt_slab_get_obj(__slab);

		/* Free in random-order */
		int cnt = 0, freed[1200];
		for (int i = 0; i < 1200; i++)
			freed[i] = 0;
		while (cnt < 1200) {
			int i = rand() % 1200;
			if (freed[i])
				continue;
			freed[i] = 1;
			cnt++;
			_tt_slab_put_obj(__slab, b[i]);
		}
	}

	return 0;
}

static int slab_test(int n)
{
	printf("Verify slab algorithm %d rounds\n", n);

	printf("alloc, free...\n");
	__slab = _tt_slab_alloc("test1", 100, 1000);
	for (int i = 0; i < n; i++) {
		int ret = slab_alloc_free();
		if (ret)
			return ret;
	}
	_tt_slab_free(__slab);

	printf("concurrency...\n");
	__slab = _tt_slab_alloc("test2", 50, 8000);
	pthread_t t[8];
	for (int i = 0; i < 8; i++)
		pthread_create(&t[i], NULL, slab_thread, (void *)(size_t)n);
	void *res;
	for (int i = 0; i < 8; i++)
		pthread_join(t[i], &res);
	if (__slab->obj_free != __slab->obj_cnt) {
		tt_error("Free size error");
		return TT_EINVAL;
	}

	return 0;
}

int main(void)
{
	srand(time(NULL));
	tt_log_set_level(TT_LOG_INFO);

#if 0
	if (buddy_test(100) == 0)
		printf("Buddy test succeeded\n");
#endif

	if (slab_test(100) == 0)
		printf("Slab test succeeded\n");

	return 0;
}
