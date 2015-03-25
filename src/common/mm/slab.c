/* Fixed size buffer management - Slab algorithm
 *
 * Copyright (C) 2015 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/common/list.h>
#include "../lib.h"

#include <pthread.h>

/* slab descriptor        slab buffer
 * +-------------+      +-------------+
 * |  obj_buf    | ---> |  object[0]  |
 * |             |      +-------------+
 * |  free_idx   |      |  object[1]  |
 * | next_idx[0] |      +-------------+
 * | next_idx[1] |      |     ...     |
 * |     ...     |      +-------------+
 * +-------------+      |  object[n]  |
 *                      +-------------+
 */

#define SLAB_MAGIC		0x28981431
#define SLAB_OBJ_DEF_CNT	128
#define SLAB_OBJ_MAX_SZ		1024
#define SLAB_BUF_MAX_SZ		(1024 * 1024)

/* Slab descriptor */
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

	int free_idx;		/* Free object index */
	int next_idx[0];	/* Next free object array
				 * - MSB=1, allocated; MSB=0, free
				 */
};

struct tt_slab *_tt_slab_alloc(const char *name, uint obj_sz, uint obj_cnt)
{
	tt_assert_fa(obj_sz);

	/* Align object size to 8 bytes boundary */
	obj_sz += 7;
	obj_sz &= ~7;

	/* Check object size and memory size */
	if (obj_sz > SLAB_OBJ_MAX_SZ) {
		tt_error("Object size must <= %d", SLAB_OBJ_MAX_SZ);
		return NULL;
	} else if (obj_sz == 0) {
		return NULL;
	}
	if (obj_cnt == 0) {
		obj_cnt = SLAB_OBJ_DEF_CNT;
		if (obj_cnt * obj_sz < 4096)
			obj_cnt = 4096 / obj_sz;
	}
	if (obj_sz * obj_cnt > SLAB_BUF_MAX_SZ) {
		obj_cnt = SLAB_BUF_MAX_SZ / obj_sz;
		tt_warn("Object count truncated to %d", obj_cnt);
	}

	/* Initialize slab descriptor and buffer */
	struct tt_slab *slab =
		malloc(sizeof(struct tt_slab) + obj_cnt * sizeof(int));
	if (!slab) {
		tt_error("Out of memory");
		return NULL;
	}
	slab->magic = SLAB_MAGIC;
	snprintf(slab->name, 16, "%s", name);
	pthread_spin_init(&slab->spl, 0);

	slab->obj_sz = obj_sz;
	slab->obj_free = slab->obj_cnt = obj_cnt;
	slab->free_idx = 0;
	for (int i = 0; i < obj_cnt-1; i++)
		slab->next_idx[i] = i + 1;
	slab->next_idx[obj_cnt-1] = ~BIT(31);

	slab->buf_sz = obj_cnt * obj_sz;
	slab->obj_buf = malloc(slab->buf_sz);
	if (!slab->obj_buf) {
		tt_error("Out of memory");
		free(slab);
		return NULL;
	}

	tt_info("Slab '%s' created: size = %d, count = %d",
			slab->name, slab->obj_sz, slab->obj_cnt);
	return slab;
}

void _tt_slab_free(struct tt_slab *slab)
{
	tt_assert_fa(slab->magic == SLAB_MAGIC);

	pthread_spin_destroy(&slab->spl);

	free(slab->obj_buf);
	free(slab);
}

void *_tt_slab_get_obj(struct tt_slab *slab)
{
	tt_assert_fa(slab->magic == SLAB_MAGIC);

	pthread_spin_lock(&slab->spl);

	/* Fallback to buddy buffer if no free object available */
	if (slab->obj_free == 0) {
		slab->missed++;
		pthread_spin_unlock(&slab->spl);
		tt_debug("Cannot get slab object: %s", slab->name);
		return _tt_get_buf(slab->obj_sz);
	}

	/* Pick free object */
	int tmp_idx = slab->next_idx[slab->free_idx];
	void *obj_ptr = slab->obj_buf + slab->free_idx * slab->obj_sz;

	/* Swap free_idx and next_idx[free_idx] */
	slab->next_idx[slab->free_idx] = slab->free_idx | BIT(31);
	slab->free_idx = tmp_idx;

	slab->obj_free--;
	tt_assert_fa((slab->obj_free == 0 && slab->free_idx == ~BIT(31)) ||
			(slab->obj_free > 0 && slab->free_idx >= 0));

	pthread_spin_unlock(&slab->spl);

	tt_assert_fa(((size_t)obj_ptr & 7) == 0);
	return obj_ptr;
}

void _tt_slab_put_obj(struct tt_slab *slab, void *obj)
{
	tt_assert_fa(slab->magic == SLAB_MAGIC);

	if (obj < slab->obj_buf || obj >= slab->obj_buf + slab->buf_sz) {
		_tt_put_buf(obj);
		return;
	}

	/* Get object index */
	size_t offset = obj - slab->obj_buf;
	tt_assert_fa(offset % slab->obj_sz == 0);
	int obj_idx = offset / slab->obj_sz;

	pthread_spin_lock(&slab->spl);

	/* Swap free_idx and next_idx[obj_idx] */
	__tt_swap(slab->free_idx, slab->next_idx[obj_idx]);
	tt_assert_fa((slab->free_idx & BIT(31)));
	slab->free_idx &= ~BIT(31);

	slab->obj_free++;

	pthread_spin_unlock(&slab->spl);
}
