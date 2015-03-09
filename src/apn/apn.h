/* Helpers
 *
 * Copyright (C) 2015 Yibo Cai
 */
#pragma once

/* Cleaer APN to 0 */
void _tt_apn_zero(struct tt_apn *apn);

/* Mapping 3 decimal to 10 bits directly in binary format.
 * From pure software point of view, I don't find any benifit of DPD encoding.
 */
static inline uint _tt_apn_d3_to_b10(uint dec3)
{
	return dec3;
}

static inline uint _tt_apn_b10_to_d3(uint bit10)
{
	return bit10;
}

uint _tt_apn_d3_to_b10_2(const uchar *dec3);
uint _tt_apn_b10_to_d3_2(uint bit10, uchar *dec3);

int _tt_apn_round_adj(struct tt_apn *apn);
