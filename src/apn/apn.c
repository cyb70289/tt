/* Helpers
 *
 * Copyright (C) 2015 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/apn/apn.h>
#include <common/lib.h>
#include "apn.h"

#include <string.h>
#include <pthread.h>

/* Creator */
struct tt_apn *tt_apn_alloc(uint prec)
{
	if (prec == 0) {
		prec = CONFIG_APN_DIGITS;
	} else if (prec < 20) {
		prec = 20;	/* int64:20, double:17 */
	} else if (prec > CONFIG_APN_DIGITS_MAX) {
		tt_error("Unsupported precision!");
		return NULL;
	}

	/* Check digit buffer size */
	int prec_full = prec;
        prec_full += TT_APN_PREC_RND;	/* Rounding guard */
	prec_full += TT_APN_PREC_CRY;	/* Carry digit */
	prec_full += TT_APN_PREC_ALN;	/* Align-9 shifting */

	/* 9 digits -> 4 bytes */
	int digsz = prec_full;
	digsz += 8;
	digsz /= 9;
	digsz *= 4;

	/* Create APN */
	struct tt_apn *apn = calloc(1, sizeof(struct tt_apn));
	apn->_dig32 = calloc(digsz / 4, 4);
	if (apn->_dig32 == NULL) {
		tt_error("Not enough memory");
		free(apn);
		return NULL;
	}
	*(int*)&apn->_prec = prec;
	*(int*)&apn->_prec_full = prec_full;
	*(uint*)&apn->_digsz = digsz;
	apn->_msb = 1;
	tt_debug("APN created: %u bytes", apn->_digsz);

	return apn;
}

void tt_apn_free(struct tt_apn *apn)
{
	free(apn->_dig32);
	free(apn);
}

/* apn = 0 */
void _tt_apn_zero(struct tt_apn *apn)
{
	apn->_sign = 0;
	apn->_inf_nan = 0;
	apn->_exp = 0;
	apn->_msb = 1;
	memset(apn->_dig32, 0, apn->_digsz);
}

/* Increase |significand|
 * apn += { apn->_sign, 1, apn->_exp }
 */
int _tt_apn_round_adj(struct tt_apn *apn)
{
	/* Construct { apn->_sign, 1, apn->_exp } */
	uint one = 1;
	struct tt_apn apn_1 = {
		._sign = apn->_sign,
		._inf_nan = 0,
		._exp = apn->_exp,
		._prec = 1,
		._digsz = sizeof(one),
		._msb = 1,
		._dig32 = &one,
	};

	return tt_apn_add(apn, apn, &apn_1);
}

/* Check sanity */
int _tt_apn_sanity(const struct tt_apn *apn)
{
	if (apn->_msb <= 0 || apn->_msb > apn->_prec)
		return TT_APN_ESANITY;

	int i, last_dig_idx = (apn->_msb + 8) / 9 - 1;

	/* Check carry guard bits */
	for (i = 0; i <= last_dig_idx; i++)
		if ((apn->_dig32[i] & (BIT(10) | BIT(21))))
			return TT_APN_ESANITY;

	/* Check remaining uints */
	for (; i < apn->_digsz / 4; i++)
		if (apn->_dig32[i])
			return TT_APN_ESANITY;

	/* Check last valid uint */
	int last_dig_cnt = (apn->_msb - 1) % 9 + 1;
        uchar d[9];
	_tt_apn_to_d3_cp(apn->_dig32[last_dig_idx] & 0x3FF, d);
	_tt_apn_to_d3_cp((apn->_dig32[last_dig_idx] >> 11) & 0x3FF, d+3);
	_tt_apn_to_d3_cp(apn->_dig32[last_dig_idx] >> 22, d+6);
	for (i = 8; i > last_dig_cnt - 1; i--)
		if (d[i])
			return TT_APN_ESANITY;
	if (d[i] == 0 && i != 0)	/* Don't blame on "0" */
		return TT_APN_ESANITY;

	/* Check zero */
	if (_tt_apn_is_zero(apn) && apn->_exp > 0)
		return TT_APN_ESANITY;

	return 0;
}

/* Get "pos-th" digit (0~9)
 * - pos starts from 0
 */
uint _tt_apn_get_dig(const uint *dig, int pos)
{
	uint dig32 = dig[pos / 9];
	pos %= 9;
	int sft = pos / 3;
	sft *= 11;

	const uchar *d = _tt_apn_to_d3((dig32 >> sft) & 0x3FF);

	return d[pos % 3];
}
