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
