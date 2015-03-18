/* Helpers
 *
 * Copyright (C) 2015 Yibo Cai
 */
#pragma once

/* Clear to zero */
void _tt_apn_zero(struct tt_apn *apn);

/* Increase abs of significand */
int _tt_apn_round_adj(struct tt_apn *apn);

/* Check if significand == 0, it's not a true zero if _exp < 0 */
static inline bool _tt_apn_is_zero(const struct tt_apn *apn)
{
	return apn->_msb == 1 && *apn->_dig32 == 0 && !apn->_inf_nan;
}

static inline bool _tt_apn_is_true_zero(const struct tt_apn *apn)
{
	return _tt_apn_is_zero(apn) && apn->_exp == 0;
}

/* Check sanity */
int _tt_apn_sanity(const struct tt_apn *apn);

/* 3 decimals to 10 bits */
static inline uint _tt_apn_from_d3(const uchar *dec3)
{
	extern const uint __lut_d3_to_b10[10][10][10];
	return __lut_d3_to_b10[dec3[2]][dec3[1]][dec3[0]];
}

/* 10 bits --> 3 decimals */
static inline const uchar *_tt_apn_to_d3(uint bit10)
{
	extern const uchar __lut_b10_to_d3[1000][4];
	return __lut_b10_to_d3[bit10];
}

static inline void _tt_apn_to_d3_cp(uint bit10, uchar *dec3)
{
	const uchar *d3 = _tt_apn_to_d3(bit10);
	dec3[0] = d3[0];
	dec3[1] = d3[1];
	dec3[2] = d3[2];
}

/* Get "pos-th" digit (pos starts from 0) */
uint _tt_apn_get_dig(const uint *dig, int pos);
