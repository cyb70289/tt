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

#if CONFIG_DEBUG_LEVEL >= 2
/* XXX: copied from common/buf.c */
struct pool {
	uint size;
	uint free;
	const void *buf;
	int blks;
	struct tt_list *heads;
	pthread_mutex_t mtx;
};

int __tt_buf_check(const struct pool **ppool);

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
		if (__tt_buf_check(&pool))
			return TT_EINVAL;
	}

	/* Free in pre-order */
	for (int i = 0; i < 1024; i++) {
		_tt_put_buf(b[i]);
		if (__tt_buf_check(&pool))
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
		if (__tt_buf_check(&pool))
			return TT_EINVAL;
	}

	/* Free in post-order */
	for (int i = 1023; i >= 0; i--) {
		_tt_put_buf(b[i]);
		if (__tt_buf_check(&pool))
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
		if (__tt_buf_check(&pool))
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
		if (__tt_buf_check(&pool))
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

int buddy_test(int n)
{
	printf("Verify buddy algorithm %d rounds\n", n);

	printf("alloc, free...\n");
	for (int i = 0; i < n; i++) {
		int ret = buddy_alloc_free();
		if (ret)
			return ret;
	}

	printf("concurrrency...\n");
	pthread_t t[8];
	for (int i = 0; i < 8; i++)
		pthread_create(&t[i], NULL, buddy_thread, (void *)(size_t)n);
	void *res;
	for (int i = 0; i < 8; i++)
		pthread_join(t[i], &res);
	/* Verify */
	const struct pool *pool;
	if (__tt_buf_check(&pool))
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

int main(void)
{
#if CONFIG_DEBUG_LEVEL >= 2
	srand(time(NULL));
	tt_log_set_level(TT_LOG_INFO);
	if (buddy_test(100) == 0)
		printf("Buddy test succeeded\n");
#endif

	return 0;
}
