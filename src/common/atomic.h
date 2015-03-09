/* Atomic operations
 *
 * Copyright (C) 2015 Yibo Cai
 */
#pragma once

#define _tt_opt_barrier()	__asm__ __volatile__("": : :"memory")

#define _tt_sync_barrier()	__sync_synchronize()

struct _tt_atomic {
	volatile int v;
};

static inline int _tt_atomic_get(const struct _tt_atomic *a)
{
	_tt_sync_barrier();
	return a->v;
}

static inline void _tt_atomic_set(struct _tt_atomic *a, int i)
{
	a->v = i;
	_tt_sync_barrier();
}

static inline int _tt_atomic_add(struct _tt_atomic *a, int i)
{
	return __sync_add_and_fetch(&a->v, i);
}

static inline int _tt_atomic_sub(struct _tt_atomic *a, int i)
{
	return __sync_sub_and_fetch(&a->v, i);
}

static inline int _tt_atomic_inc(struct _tt_atomic *a)
{
	return _tt_atomic_add(a, 1);
}

static inline int _tt_atomic_dec(struct _tt_atomic *a)
{
	return _tt_atomic_sub(a, 1);
}

static inline int _tt_atomic_or(struct _tt_atomic *a, int i)
{
	return __sync_or_and_fetch(&a->v, i);
}

static inline int _tt_atomic_and(struct _tt_atomic *a, int i)
{
	return __sync_and_and_fetch(&a->v, i);
}

static inline int _tt_atomic_xor(struct _tt_atomic *a, int i)
{
	return __sync_xor_and_fetch(&a->v, i);
}
