/* Key functions
 * XXX: alignment?
 *
 * Copyright (C) 2014 Yibo Cai
 */
#include <tt/tt.h>

#include <string.h>

/* Swap */
static void tt_swap_8(const struct tt_key *num, void *v1, void *v2)
{
	int8_t tmp = *(int8_t *)v1;
	*(int8_t *)v1 = *(int8_t *)v2;
	*(int8_t *)v2 = tmp;
}

static void tt_swap_16(const struct tt_key *num, void *v1, void *v2)
{
	int16_t tmp = *(int16_t *)v1;
	*(int16_t *)v1 = *(int16_t *)v2;
	*(int16_t *)v2 = tmp;
}

static void tt_swap_32(const struct tt_key *num, void *v1, void *v2)
{
	int32_t tmp = *(int32_t *)v1;
	*(int32_t *)v1 = *(int32_t *)v2;
	*(int32_t *)v2 = tmp;
}

static void tt_swap_64(const struct tt_key *num, void *v1, void *v2)
{
	int64_t tmp = *(int64_t *)v1;
	*(int64_t *)v1 = *(int64_t *)v2;
	*(int64_t *)v2 = tmp;
}

static void tt_swap_general(const struct tt_key *num, void *v1, void *v2)
{
	int size4 = num->size / 4;
	int size1 = num->size % 4;

	while (size4--) {
		int32_t tmp = *(int32_t *)v1;
		*(int32_t *)v1 = *(int32_t *)v2;
		*(int32_t *)v2 = tmp;
		v1 += 4;
		v2 += 4;
	}

	while (size1--) {
		int8_t tmp = *(int8_t *)v1;
		*(int8_t *)v1 = *(int8_t *)v2;
		*(int8_t *)v2 = tmp;
		v1++;
		v2++;
	}
}

static void (*tt_swap_select(uint size))
	(const struct tt_key *, void *, void *)
{
	switch (size) {
	case 1:
		tt_debug("8-bit swap");
		return tt_swap_8;
	case 2:
		tt_debug("16-bit swap");
		return tt_swap_16;
	case 4:
		tt_debug("32-bit swap");
		return tt_swap_32;
	case 8:
		tt_debug("64-bit swap");
		return tt_swap_64;
	default:
		tt_debug("General swap");
		return tt_swap_general;
	}
}

/* Set */
static void tt_set_8(const struct tt_key *num, void *dst, const void *src)
{
	*(int8_t *)dst = *(int8_t *)src;
}

static void tt_set_16(const struct tt_key *num, void *dst, const void *src)
{
	*(int16_t *)dst = *(int16_t *)src;
}

static void tt_set_32(const struct tt_key *num, void *dst, const void *src)
{
	*(int32_t *)dst = *(int32_t *)src;
}

static void tt_set_64(const struct tt_key *num, void *dst, const void *src)
{
	*(int64_t *)dst = *(int64_t *)src;
}

static void tt_set_general(const struct tt_key *num, void *dst, const void *src)
{
	memcpy(dst, src, num->size);
}

static void (*tt_set_select(uint size))
	(const struct tt_key *, void *, const void *)
{
	switch (size) {
	case 1:
		tt_debug("8-bit set");
		return tt_set_8;
	case 2:
		tt_debug("16-bit set");
		return tt_set_16;
	case 4:
		tt_debug("32-bit set");
		return tt_set_32;
	case 8:
		tt_debug("64-bit set");
		return tt_set_64;
	default:
		tt_info("General set");
		return tt_set_general;
	}
}

/* Select default swap, set routines */
int _tt_key_select(struct tt_key *num)
{
	if (!num->swap)
		num->swap = tt_swap_select(num->size);
	if (!num->cmp) {
		tt_error("No compare routine");
		return TT_EINVAL;
	}
	num->_set = tt_set_select(num->size);

	return 0;
}
