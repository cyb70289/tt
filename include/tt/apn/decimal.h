/* Arbitrary precision decimal
 *
 * Copyright (C) 2015 Yibo Cai
 */
#pragma once

struct tt_dec;

struct tt_dec *tt_dec_alloc(uint prec);
void tt_dec_free(struct tt_dec *dec);

/* Conversion */
int tt_dec_from_string(struct tt_dec *dec, const char *str);
int tt_dec_from_sint(struct tt_dec *dec, int64_t num);
int tt_dec_from_uint(struct tt_dec *dec, uint64_t num);
int tt_dec_from_float(struct tt_dec *dec, double num);
int tt_dec_to_string(const struct tt_dec *dec, char *str, uint len);
#ifdef __STDC_IEC_559__
int tt_dec_to_float(const struct tt_dec *dec, double *num);
#endif

/* Operations */
int tt_dec_add(struct tt_dec *dst, const struct tt_dec *src1,
		const struct tt_dec *src2);
int tt_dec_sub(struct tt_dec *dst, const struct tt_dec *src1,
		const struct tt_dec *src2);
int tt_dec_mul(struct tt_dec *dst, const struct tt_dec *src1,
		const struct tt_dec *src2);
int tt_dec_div(struct tt_dec *dst, const struct tt_dec *src1,
		const struct tt_dec *src2);

/* Logical */
int tt_dec_cmp(const struct tt_dec *src1, const struct tt_dec *src2);
int tt_dec_cmp_abs(const struct tt_dec *src1, const struct tt_dec *src2);
