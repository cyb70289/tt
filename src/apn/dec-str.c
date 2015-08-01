/* Decimal <--> Scientific notation string
 *
 * Copyright (C) 2015 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/apn/decimal.h>
#include <tt/common/round.h>
#include <common/lib.h>
#include "decimal.h"

#include <string.h>
#include <math.h>

static int parse_coef(struct tt_dec *dec, const char *s, int len, int *adjexp,
		int *adjrnd)
{
	int ret = 0;

	/* Check syntax, point position, msb */
	bool nopoint = true;
	int ptpos = len - 1, msb = len - 1;
	for (int i = 0; i < len; i++) {
		if (s[i] == '.') {
			nopoint = false;
			if (ptpos == (len - 1))
				ptpos = i;
			else
				return TT_EINVAL;	/* More than one "." */
		} else if (s[i] >= '0' && s[i] <= '9') {
			if (msb == (len - 1) && s[i] != '0')
				msb = i;
		} else {
			return TT_EINVAL;
		}
	}

	/* Check exponent adjust, significand */
	*adjexp = -(len - ptpos - 1);
	int digs = len - msb;
	if (!nopoint && ptpos > msb)
		digs--;		/* Exclude "." from digits */
	dec->_msb = digs;

	/* Check rounding */
	const char *s2 = s + len - 1;
	if (digs > dec->_prec) {
		tt_info("DEC rounded: %d -> %d", digs, dec->_prec);
		ret = TT_APN_EROUNDED;

		/* Drop trailing digits */
		int drops = digs - dec->_prec;
		dec->_msb -= drops;
		*adjexp += drops;
		while (s2 > s) {
			if (*s2 != '.')
				if (--drops == 0)
					break;
			s2--;
		}
		s2--;
		tt_assert(s2 >= s && drops == 0);

		/* Check round adjust */
		const char *ssig = s2;
		const char *srnd = s2 + 1;
		while (*ssig == '.')
			ssig--;
		while (*srnd == '.')
			srnd++;
		tt_assert(*ssig >= '0' && *ssig <= '9' &&
				*srnd >= '0' && *ssig <= '9');
		*adjrnd = _tt_round((*ssig - '0') & 1, *srnd - '0', 0);
	}

	/* Conversion: digits in [s, s2], may contain one point */
	uint *dig32 = dec->_dig32;
	uint dig = 0, n10 = 1;
	int cnt = 0;
	while (s2 >= s) {
		if (*s2 != '.') {
			dig += (*s2 - '0') * n10;
			cnt++;
			n10 *= 10;
			if (cnt == 9) {
				*dig32++ = dig;
				cnt = 0;
				dig = 0;
				n10 = 1;
			}
		}
		s2--;
	}
	/* Partial digits at MSB */
	if (cnt)
		*dig32 = dig;

	return ret;
}

int tt_dec_from_string(struct tt_dec *dec, const char *str)
{
	int ret;

	_tt_dec_zero(dec);

	/* Check leading "+", "-" */
	if (*str == '-') {
		dec->_sign = 1;
		str++;
	} else if (*str == '+') {
		str++;
	}

	/* Check "NaN", "Inf" */
	if (strcasecmp(str, "nan") == 0) {
		dec->_inf_nan = TT_DEC_NAN;
		return 0;
	} else if (strcasecmp(str, "inf") == 0) {
		dec->_inf_nan = TT_DEC_INF;
		return 0;
	}

	/* Cut coefficient */
	int epos = 0;
	while (str[epos]) {
		if (str[epos] == 'E' || str[epos] == 'e')
			break;
		epos++;
	}
	if (epos == 0)
		goto invalid;

	/* Parse coefficient */
	int adjexp = 0, adjrnd = 0;
	ret = parse_coef(dec, str, epos, &adjexp, &adjrnd);
	if (ret < 0)
		goto invalid;

	/* Parse exponent. TODO: xflow */
	int _exp = 0;
	if (str[epos]) {
		if (_tt_atoi(str+epos+1, &_exp))
			goto invalid;
	}
	dec->_exp = _exp + adjexp;

	/* Rounding */
	if (adjrnd) {
		uint one = 1;
		struct tt_dec dec_1 = {
			._sign = dec->_sign,
			._inf_nan = 0,
			._exp = dec->_exp,
			._prec = 1,
			._digsz = sizeof(one),
			._msb = 1,
			._dig32 = &one,
		};
		tt_dec_add(dec, dec, &dec_1);
	}

	/* Check zero */
	if (_tt_dec_is_zero(dec) && dec->_exp > 0)
		dec->_exp = 0;

	return ret;

invalid:
	_tt_dec_zero(dec);
	return TT_EINVAL;
}

int tt_dec_to_string(const struct tt_dec *dec, char *str, uint len)
{
	tt_assert(len);
	*str = '\0';

	/* Required buffer length */
	uint len2 = 1;	/* '\0' */

	/* Check Sign */
	if (dec->_sign) {
		*str++ = '-';
		len2++;
	}

	/* Check NaN, Inf */
	if (dec->_inf_nan) {
		len2 += 3;	/* "NaN", "Inf" */
		if (len2 > len)
			return TT_ENOBUFS;
		strcpy(str, dec->_inf_nan == TT_DEC_NAN ? "NaN" : "Inf");
		return 0;
	}

	len2 += dec->_msb;

	/* Scientific notation. digits => abcd */
	int note = 0;	/* abcd */
	int ptpos = 0;	/* point position from msb */
	int zeros = 0;	/* Prepending zeros when note == 1 */
	const int adjexp = dec->_exp + dec->_msb - 1;	/* TODO: xflow */
	if (dec->_exp > 0 || adjexp < -6) {
		if (dec->_msb == 1) {
			note = 3;	/* aEadjexp */
		} else {
			note = 2;	/* a.bcdEadjexp */
			len2++;		/* "." */
			ptpos = 1;
		}
		len2 += 2;		/* "E+", "E-" */
		len2 += _tt_digits_ll(llabs(adjexp));
	} else if (dec->_exp < 0) {
		note = 1;	/* 0.0abcd, ab.cd (|exp| digits after ".") */
		len2++;		/* "." */
		zeros = abs(dec->_exp) - dec->_msb + 1;
		if (zeros > 0)
			len2 += zeros;
		else
			ptpos = -zeros + 1;
	}

	if (len2 > len) {
		snprintf(str, len, "Not enough space");
		return TT_ENOBUFS;
	}

	/* Prepend 0 when note == 1 */
	if (zeros > 0) {
		*str++ = '0';
		*str++ = '.';
		while (--zeros)
			*str++ = '0';
	}

	/* Get valid uints */
	int uints = (dec->_msb + 8) / 9;

	/* Generate coefficient */
	bool skip0 = true;
	const uint *dig32 = dec->_dig32 + uints - 1;
	int digs = 0;
	while (digs < dec->_msb) {
		/* Change to 9 decimals */
		uchar dec9[9];
		_tt_dec_to_d9(*dig32, dec9);
		dig32--;

		for (int i = 8; i >= 0; i--) {
			if (skip0 && dec9[i] == 0 && i)
				continue;
			else
				skip0 = false;
			*str++ = dec9[i] + '0';
			if (++digs == ptpos)
				*str++ = '.';
		}
	}
	tt_assert(digs == dec->_msb);

	/* Generate exponent */
	if (note >= 2) {
		/* Append "Eadjexp" */
		*str++ = 'E';
		sprintf(str, "%+d", adjexp);
	} else {
		*str = '\0';
	}

	return 0;
}
