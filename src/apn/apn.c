/* Helpers
 *
 * Copyright (C) 2015 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/apn/apn.h>
#include <common/atomic.h>
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

	/* 9 digits -> 4 bytes */
	uint digsz = prec + TT_APN_EXT_PREC;	/* Extra prec for rounding */
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
	*(uint*)&apn->_prec = prec;
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

/* 3 decimals to 10 bits */
uint _tt_apn_from_d3(const uchar *dec3)
{
	uint bit10 = 0;

	for (int i = 2; i >= 0; i--) {
		bit10 *= 10;
		bit10 += dec3[i];
	}
	tt_assert_fa(bit10 < 1000);

	return bit10;
}

/* 10 bits --> 3 decimals */
void _tt_apn_to_d3(uint bit10, uchar *dec3)
{
	tt_assert_fa(bit10 < 1000);

	dec3[2] = bit10 / 100;
	bit10 %= 100;
	dec3[1] = bit10 / 10;
	dec3[0] = bit10 % 10;
}

/* Increase |significand|
 * apn += { apn->_sign, 1, apn->_exp }
 */
int _tt_apn_round_adj(struct tt_apn *apn)
{
	/* TODO */
	return 0;
}
