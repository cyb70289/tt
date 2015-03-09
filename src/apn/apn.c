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

	/* 3 digits -> 10 bits */
	uint digsz = prec;
	digsz += 2;
	digsz /= 3;
	digsz *= 10;
	/* bits -> bytes */
	digsz --;
	digsz /= 8;
	digsz++;
	/* Pad to 4n bytes */
	digsz += 3;
	digsz &= ~3;

	/* Create APN */
	struct tt_apn *apn = calloc(1, sizeof(struct tt_apn) + digsz);
	if (apn == NULL) {
		tt_error("Not enough memory");
		return NULL;
	}
	apn->_msb = 1;
	apn->_prec = prec;
	apn->_digsz = digsz;
	tt_debug("APN created: %u bytes", apn->_digsz);

	return apn;
}

/* apn = 0 */
void _tt_apn_zero(struct tt_apn *apn)
{
	apn->_sign = 0;
	apn->_inf_nan = 0;
	apn->_exp = 0;
	apn->_msb = 1;
	memset(apn->_dig8, 0, apn->_digsz);
}

/* 3 decimals to 10 bits */
uint _tt_apn_d3_to_b10_2(const uchar *dec3)
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
uint _tt_apn_b10_to_d3_2(uint bit10, uchar *dec3)
{
	tt_assert_fa(bit10 < 1000);

	/* Lookup table created on demand */
	static const uchar (*bit10_to_dec3)[3];
	static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

	/* Create table on first lookup */
	if (bit10_to_dec3 == NULL) {
		pthread_mutex_lock(&mtx);

		_tt_sync_barrier();
		if (bit10_to_dec3 == NULL) {
			uchar (*tbl)[3] = malloc(1000 * sizeof(*bit10_to_dec3));
			for (int i = 0; i < 1000; i++) {
				int n = i;
				tbl[i][0] = n % 10;
				n /= 10;
				tbl[i][1] = n % 10;
				tbl[i][2] = n / 10;
			}
			bit10_to_dec3 = (const uchar (*)[3])tbl;
			_tt_sync_barrier();
		}

		pthread_mutex_unlock(&mtx);
	}

	dec3[0] = bit10_to_dec3[bit10][0];
	dec3[1] = bit10_to_dec3[bit10][1];
	dec3[2] = bit10_to_dec3[bit10][2];

	return bit10;
}

/* Increase |significand|
 * apn += { apn->_sign, 1, apn->_exp }
 */
int _tt_apn_round_adj(struct tt_apn *apn)
{
	/* TODO */
	return 0;
}
