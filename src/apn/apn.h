/* Helpers
 *
 * Copyright (C) 2015 Yibo Cai
 */
#pragma once

/* Clear zero */
void _tt_apn_zero(struct tt_apn *apn);

/* Increase abs of significand */
int _tt_apn_round_adj(struct tt_apn *apn);

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
