/* Arbitrary precision number
 *
 * Copyright (C) 2015 Yibo Cai
 */
#pragma once

struct tt_apn;

struct tt_apn *tt_apn_alloc(uint prec);
void tt_apn_free(struct tt_apn *apn);

/* Conversion */
int tt_apn_from_string(struct tt_apn *apn, const char *str);
int tt_apn_from_sint(struct tt_apn *apn, int64_t num);
int tt_apn_from_uint(struct tt_apn *apn, uint64_t num);
int tt_apn_from_float(struct tt_apn *apn, double num);
int tt_apn_to_string(const struct tt_apn *apn, char *str, uint len);
#ifdef __STDC_IEC_559__
int tt_apn_to_float(const struct tt_apn *apn, double *num);
#endif

/* Operations */
int tt_apn_add(struct tt_apn *dst, const struct tt_apn *src1,
		const struct tt_apn *src2);
int tt_apn_sub(struct tt_apn *dst, const struct tt_apn *src1,
		const struct tt_apn *src2);
int tt_apn_mul(struct tt_apn *dst, const struct tt_apn *src1,
		const struct tt_apn *src2);
int tt_apn_div(struct tt_apn *dst, const struct tt_apn *src1,
		const struct tt_apn *src2);

/* Logical */
int tt_apn_cmp(const struct tt_apn *src1, const struct tt_apn *src2);
int tt_apn_cmp_abs(const struct tt_apn *src1, const struct tt_apn *src2);
