/* mm.h
 *
 * Copyright (C) 2015 Yibo Cai
 */
#pragma once

static inline void *_tt_get_buf(size_t sz)
{
	return malloc(sz);
}

static inline void _tt_put_buf(void *buf)
{
	free(buf);
}

struct tt_slab;
struct tt_slab *_tt_slab_alloc(const char *name, uint obj_sz, uint obj_cnt);
void _tt_slab_free(struct tt_slab *slab);
void *_tt_slab_get_obj(struct tt_slab *slab);
void _tt_slab_put_obj(struct tt_slab *slab, void *obj);
