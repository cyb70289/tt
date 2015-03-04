/* Atomic operations
 *
 * Copyright (C) 2015 Yibo Cai
 */
#pragma once

#define tt_opt_barrier()	__asm__ __volatile__("": : :"memory")

#define tt_sync_barrier()	__sync_synchronize()

struct tt_atomic {
	volatile int v;
};

static inline int tt_atomic_get(const struct tt_atomic *a)
{
	tt_sync_barrier();
	return a->v;
}

static inline void tt_atomic_set(struct tt_atomic *a, int i)
{
	a->v = i;
	tt_sync_barrier();
}

static inline int tt_atomic_add(struct tt_atomic *a, int i)
{
	return __sync_add_and_fetch(&a->v, i);
}

static inline int tt_atomic_sub(struct tt_atomic *a, int i)
{
	return __sync_sub_and_fetch(&a->v, i);
}

static inline int tt_atomic_inc(struct tt_atomic *a)
{
	return tt_atomic_add(a, 1);
}

static inline int tt_atomic_dec(struct tt_atomic *a)
{
	return tt_atomic_sub(a, 1);
}

static inline int tt_atomic_or(struct tt_atomic *a, int i)
{
	return __sync_or_and_fetch(&a->v, i);
}

static inline int tt_atomic_and(struct tt_atomic *a, int i)
{
	return __sync_and_and_fetch(&a->v, i);
}

static inline int tt_atomic_xor(struct tt_atomic *a, int i)
{
	return __sync_xor_and_fetch(&a->v, i);
}
