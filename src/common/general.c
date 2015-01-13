/* General swap, comapre, set
 *
 * Copyright (C) 2014 Yibo Cai
 */
#include <tt/tt.h>
#include "_common.h"

#include <string.h>

/* Swap */
static void tt_swap_8(const struct tt_num *num, void *v1, void *v2)
{
	int8_t tmp = *(int8_t *)v1;
	*(int8_t *)v1 = *(int8_t *)v2;
	*(int8_t *)v2 = tmp;
}

static void tt_swap_16(const struct tt_num *num, void *v1, void *v2)
{
	int16_t tmp = *(int16_t *)v1;
	*(int16_t *)v1 = *(int16_t *)v2;
	*(int16_t *)v2 = tmp;
}

static void tt_swap_32(const struct tt_num *num, void *v1, void *v2)
{
	int32_t tmp = *(int32_t *)v1;
	*(int32_t *)v1 = *(int32_t *)v2;
	*(int32_t *)v2 = tmp;
}

static void tt_swap_64(const struct tt_num *num, void *v1, void *v2)
{
	int64_t tmp = *(int64_t *)v1;
	*(int64_t *)v1 = *(int64_t *)v2;
	*(int64_t *)v2 = tmp;
}

static void (*tt_swap_select(uint size))
	(const struct tt_num *, void *, void *)
{
	switch (size) {
	case 1:
		tt_debug("8-bits");
		return tt_swap_8;
	case 2:
		tt_debug("16-bits");
		return tt_swap_16;
	case 4:
		tt_debug("32-bits");
		return tt_swap_32;
	case 8:
		tt_debug("64-bits");
		return tt_swap_64;
	default:
		tt_info("Unknown data size");
		return NULL;
	}
}

/* Compare */
static int tt_cmp_s8(const struct tt_num *num, const void *v1, const void *v2)
{
	return *(int8_t *)v1 - *(int8_t *)v2;
}

static int tt_cmp_s16(const struct tt_num *num, const void *v1, const void *v2)
{
	return *(int16_t *)v1 - *(int16_t *)v2;
}

static int tt_cmp_s32(const struct tt_num *num, const void *v1, const void *v2)
{
	return *(int32_t *)v1 - *(int32_t *)v2;
}

static int tt_cmp_s64(const struct tt_num *num, const void *v1, const void *v2)
{
	return *(int64_t *)v1 - *(int64_t *)v2;
}

/* Compare two unsigned values
 * +-----------+-----------+----------------+---------+
 * | MSB of v1 | MSB of v2 | MSB of (v1-v2) | Result  |
 * +-----------+-----------+----------------+---------+
 * |     1     |     0     |       x        | v1 > v2 |
 * +-----------+-----------+----------------+---------+
 * |     0     |     1     |       x        | v1 < v2 |
 * +-----------+-----------+----------------+---------+
 * |           |           |       0        | v1 > v2 |
 * |     1     |     1     +----------------+---------+
 * |           |           |       1        | v1 < v2 |
 * +-----------+-----------+----------------+---------+
 * |           |           |       1        | v1 < v2 |
 * |     0     |     0     +----------------+---------+
 * |           |           |       0        | v1 > v2 |
 * +-----------+-----------+----------------+---------+
 *
 * Result = ((v1 ^ v2) & v2) | (~(v1 ^ v2)) & (v1 - v2))
 * - If v1, v2 is of same sign, return v1 - v2
 * - Otherwise, return v2
 */
static int tt_cmp_u8(const struct tt_num *num, const void *v1, const void *v2)
{
	uint8_t u1 = *(uint8_t *)v1;
	uint8_t u2 = *(uint8_t *)v2;
#if 0
	uint8_t ut = u1 ^ u2;
	return (ut & u2) | ((~ut) & (u1 - u2));
#else
	return u1 < u2 ? -1 : ((u1 - u2) >> 1);
#endif
}

static int tt_cmp_u16(const struct tt_num *num, const void *v1, const void *v2)
{
	uint16_t u1 = *(uint16_t *)v1;
	uint16_t u2 = *(uint16_t *)v2;
	return u1 < u2 ? -1 : ((u1 - u2) >> 1);
}

static int tt_cmp_u32(const struct tt_num *num, const void *v1, const void *v2)
{
	uint32_t u1 = *(uint32_t *)v1;
	uint32_t u2 = *(uint32_t *)v2;
	return u1 < u2 ? -1 : ((u1 - u2) >> 1);
}

static int tt_cmp_u64(const struct tt_num *num, const void *v1, const void *v2)
{
	uint64_t u1 = *(uint64_t *)v1;
	uint64_t u2 = *(uint64_t *)v2;
	return u1 < u2 ? -1 : ((u1 - u2) >> 1);
}

static int tt_cmp_float(const struct tt_num *num,
		const void *v1, const void *v2)
{
	float f = *(float *)v1 - *(float *)v2;
	if (_tt_is_zero(f))
		return 0;
	else if (f < 0)
		return -1;
	else
		return 1;
}

static int tt_cmp_double(const struct tt_num *num,
		const void *v1, const void *v2)
{
	double f = *(double *)v1 - *(double *)v2;
	if (_tt_is_zero(f))
		return 0;
	else if (f < 0)
		return -1;
	else
		return 1;
}

static int (*tt_cmp_select(uint size, enum tt_num_type type))
	(const struct tt_num *, const void *, const void *)
{
	if (type == TT_NUM_FLOAT) {
		tt_debug("%s", size == sizeof(double) ? "double" : "float");
		return size == sizeof(double) ? tt_cmp_double : tt_cmp_float;
	}

	const char *sign = "signed";
	if (type == TT_NUM_UNSIGNED)
		sign = "unsigned";

	switch (size) {
	case 1:
		tt_debug("8-bits %s", sign);
		return type == TT_NUM_SIGNED ? tt_cmp_s8 : tt_cmp_u8;
	case 2:
		tt_debug("16-bits %s", sign);
		return type == TT_NUM_SIGNED ? tt_cmp_s16 : tt_cmp_u16;
	case 4:
		tt_debug("32-bits %s", sign);
		return type == TT_NUM_SIGNED ? tt_cmp_s32 : tt_cmp_u32;
	case 8:
		tt_debug("64-bits %s", sign);
		return type == TT_NUM_SIGNED ? tt_cmp_s64 : tt_cmp_u64;
	default:
		tt_info("Unknown data size");
		return NULL;
	}
}

/* Set */
static void tt_set_8(const struct tt_num *num, void *dst, const void *src)
{
	*(int8_t *)dst = *(int8_t *)src;
}

static void tt_set_16(const struct tt_num *num, void *dst, const void *src)
{
	*(int16_t *)dst = *(int16_t *)src;
}

static void tt_set_32(const struct tt_num *num, void *dst, const void *src)
{
	*(int32_t *)dst = *(int32_t *)src;
}

static void tt_set_64(const struct tt_num *num, void *dst, const void *src)
{
	*(int64_t *)dst = *(int64_t *)src;
}

static void tt_set_general(const struct tt_num *num, void *dst, const void *src)
{
	memcpy(dst, src, num->size);
}

static void (*tt_set_select(uint size))
	(const struct tt_num *, void *, const void *)
{
	switch (size) {
	case 1:
		tt_debug("8-bits");
		return tt_set_8;
	case 2:
		tt_debug("16-bits");
		return tt_set_16;
	case 4:
		tt_debug("32-bits");
		return tt_set_32;
	case 8:
		tt_debug("64-bits");
		return tt_set_64;
	default:
		tt_info("General memcpy");
		return tt_set_general;
	}
}

/* Set default swap, compare, set routines */
int _tt_num_select(struct tt_num *num)
{
	if (!num->swap) {
		num->swap = tt_swap_select(num->size);
		if (!num->swap) {
			tt_error("No default swap routine");
			return -TT_EPARAM;
		}
	}
	if (!num->cmp) {
		num->cmp = tt_cmp_select(num->size, num->type);
		if (!num->cmp) {
			tt_error("No default compare routine");
			return -TT_EPARAM;
		}
	}
	num->_set = tt_set_select(num->size);

	return 0;
}
