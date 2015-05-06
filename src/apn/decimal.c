/* Helpers of decimal operations
 *
 * Copyright (C) 2015 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/apn/decimal.h>
#include <common/lib.h>
#include "decimal.h"

#include <string.h>
#include <pthread.h>

/* Creator */
struct tt_dec *tt_dec_alloc(uint prec)
{
	if (prec == 0) {
		prec = CONFIG_DEC_DIGITS;
	} else if (prec < 20) {
		prec = 20;	/* int64:20, double:17 */
	} else if (prec > CONFIG_DEC_DIGITS_MAX) {
		tt_error("Unsupported precision!");
		return NULL;
	}

	/* Check digit buffer size */
	int prec_full = prec;
        prec_full += TT_DEC_PREC_RND;	/* Rounding guard */
	prec_full += TT_DEC_PREC_CRY;	/* Carry digit */
	prec_full += TT_DEC_PREC_ALN;	/* Align-9 shifting */

	/* 9 digits -> 4 bytes */
	int digsz = prec_full;
	digsz += 8;
	digsz /= 9;

	/* Create decimal */
	struct tt_dec *dec = calloc(1, sizeof(struct tt_dec));
	dec->_dig32 = calloc(digsz, 4);
	if (dec->_dig32 == NULL) {
		tt_error("Not enough memory");
		free(dec);
		return NULL;
	}
	*(int*)&dec->_prec = prec;
	*(int*)&dec->_prec_full = prec_full;
	*(uint*)&dec->_digsz = digsz * 4;
	dec->_msb = 1;
	tt_debug("Decimal created: %u bytes", dec->_digsz);

	return dec;
}

void tt_dec_free(struct tt_dec *dec)
{
	free(dec->_dig32);
	free(dec);
}

/* dec = 0 */
void _tt_dec_zero(struct tt_dec *dec)
{
	dec->_sign = 0;
	dec->_inf_nan = 0;
	dec->_exp = 0;
	dec->_msb = 1;
	memset(dec->_dig32, 0, dec->_digsz);
}

/* Check sanity */
int _tt_dec_sanity(const struct tt_dec *dec)
{
	if (dec->_msb <= 0 || dec->_msb > dec->_prec)
		return TT_APN_ESANITY;

	int i, last_dig_idx = (dec->_msb + 8) / 9 - 1;

	/* Check carry guard bits */
	for (i = 0; i <= last_dig_idx; i++)
		if ((dec->_dig32[i] & (BIT(30) | BIT(31))))
			return TT_APN_ESANITY;

	/* Check remaining uints */
	for (; i < dec->_digsz / 4; i++)
		if (dec->_dig32[i])
			return TT_APN_ESANITY;

	/* Check last valid uint */
	int last_dig_cnt = (dec->_msb - 1) % 9 + 1;
        uchar d[9];
	_tt_dec_to_d9(dec->_dig32[last_dig_idx], d);
	for (i = 8; i > last_dig_cnt - 1; i--)
		if (d[i])
			return TT_APN_ESANITY;
	if (d[i] == 0 && i != 0)	/* Don't blame on "0" */
		return TT_APN_ESANITY;

	/* Check zero */
	if (_tt_dec_is_zero(dec) && dec->_exp > 0)
		return TT_APN_ESANITY;

	return 0;
}

/* Get "pos-th" digit (0~9)
 * - pos starts from 0
 */
uint _tt_dec_get_dig(const uint *dig, int pos)
{
	uint dig32 = dig[pos / 9];
	pos %= 9;

	while (pos--)
		dig32 /= 10;

	return dig32 % 10;
}

/* Convert uint64 to decimal
 * - dig must be zeroed
 * - return MSB
 */
int _tt_dec_uint_to_dec(uint *dig32, uint64_t num)
{
	int msb = 1;
	if (num)
		msb = 0;	/* Compensate "0" */

	while (num) {
		/* Get 9 decimals */
		uint dec9 = num % 1000000000;
		num /= 1000000000;

		*dig32++ = dec9;

		/* Increment significand */
		if (num)
			msb += 9;
		else
			do {
				msb++;
				dec9 /= 10;
			} while (dec9);
	}

	return msb;
}

/* Convert uint128 to decimal
 * - dig must be zeroed
 * - return MSB
 */
#ifdef __SIZEOF_INT128__
int _tt_dec_uint128_to_dec(uint *dig32, __uint128_t num)
{
	int msb = 1;
	if (num)
		msb = 0;	/* Compensate "0" */

	while (num) {
		/* Get 9 decimals */
		uint dec9 = num % 1000000000;
		num /= 1000000000;

		*dig32++ = dec9;

		/* Increment significand */
		if (num)
			msb += 9;
		else
			do {
				msb++;
				dec9 /= 10;
			} while (dec9);
	}

	return msb;
}
#endif
