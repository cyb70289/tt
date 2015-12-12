/* Arbitrary precision integer
 *
 * Copyright (C) 2015 Yibo Cai
 */
#pragma once

struct tt_int;

struct tt_int *tt_int_alloc(void);
void tt_int_free(struct tt_int *ti);

/* Conversion */
int tt_int_from_string(struct tt_int *ti, const char *str);
int tt_int_from_sint(struct tt_int *ti, int64_t num);
int tt_int_from_uint(struct tt_int *ti, uint64_t num);
int tt_int_to_string(const struct tt_int *ti, char **str, int radix);

/* Operations */
int tt_int_add(struct tt_int *dst, const struct tt_int *src1,
		const struct tt_int *src2);
int tt_int_sub(struct tt_int *dst, const struct tt_int *src1,
		const struct tt_int *src2);
int tt_int_mul(struct tt_int *dst, const struct tt_int *src1,
		const struct tt_int *src2);
int tt_int_div(struct tt_int *quo, struct tt_int *rem,
		const struct tt_int *src1, const struct tt_int *src2);

/* Logical */
int tt_int_cmp(const struct tt_int *src1, const struct tt_int *src2);
int tt_int_cmp_abs(const struct tt_int *src1, const struct tt_int *src2);

/* Math */
int tt_int_factorial(struct tt_int *ti, const int n);

/* Number theory */
int tt_int_isprime(const struct tt_int *ti);
