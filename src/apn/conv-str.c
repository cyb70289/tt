/* APN <--> Scientific notation string
 *
 * Copyright (C) 2015 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/apn/apn.h>
#include <tt/common/round.h>
#include <common/lib.h>
#include "apn.h"

#include <string.h>
#include <math.h>

static void d3_fill(const uchar *dec3, uint **dig32, int *ptr)
{
	uint bit10 = _tt_apn_from_d3(dec3);
	**dig32 |= (bit10 << *ptr);
	*ptr += 11;
	if (*ptr > 32) {
		*ptr = 0;
		(*dig32)++;
	}
}

static int parse_coef(struct tt_apn *apn, const char *s, int len, int *adjexp,
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
	apn->_msb = digs;

	/* Check rounding */
	const char *s2 = s + len - 1;
	if (digs > apn->_prec) {
		tt_warn("APN rounded: %d -> %d", digs, apn->_prec);
		ret = TT_APN_EROUNDED;

		/* Drop trailing digits */
		int drops = digs - apn->_prec;
		apn->_msb -= drops;
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
	uint *dig32 = apn->_dig32;
	uchar dec3[3];
	int ptr = 0, cnt = 0;
	while (s2 >= s) {
		if (*s2 != '.') {
			dec3[cnt++] = (*s2 - '0');
			if (cnt == 3) {
				cnt = 0;
				d3_fill(dec3, &dig32, &ptr);
			}
		}
		s2--;
	}
	/* Partial digits at MSB */
	if (cnt) {
		while (cnt < 3)
			dec3[cnt++] = 0;
		d3_fill(dec3, &dig32, &ptr);
	}

	return ret;
}

int tt_apn_from_string(struct tt_apn *apn, const char *str)
{
	int ret;

	_tt_apn_zero(apn);

	/* Check leading "+", "-" */
	if (*str == '-') {
		apn->_sign = 1;
		str++;
	} else if (*str == '+') {
		str++;
	}

	/* Check "NaN", "Inf" */
	if (strcasecmp(str, "nan") == 0) {
		apn->_inf_nan = TT_APN_NAN;
		return 0;
	} else if (strcasecmp(str, "inf") == 0) {
		apn->_inf_nan = TT_APN_INF;
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
	ret = parse_coef(apn, str, epos, &adjexp, &adjrnd);
	if (ret < 0)
		goto invalid;

	/* Parse exponent. TODO: xflow */
	int _exp = 0;
	if (str[epos]) {
		if (_tt_atoi(str+epos+1, &_exp))
			goto invalid;
	}
	apn->_exp = _exp + adjexp;

	/* Rounding */
	if (adjrnd)
		_tt_apn_round_adj(apn);

	return ret;

invalid:
	_tt_apn_zero(apn);
	return TT_EINVAL;
}

int tt_apn_to_string(const struct tt_apn *apn, char *str, uint len)
{
	tt_assert(len);
	*str = '\0';

	/* Required buffer length */
	uint len2 = 1;	/* '\0' */

	/* Check Sign */
	if (apn->_sign) {
		*str++ = '-';
		len2++;
	}

	/* Check NaN, Inf */
	if (apn->_inf_nan) {
		len2 += 3;	/* "NaN", "Inf" */
		if (len2 > len)
			return TT_ENOBUFS;
		strcpy(str, apn->_inf_nan == TT_APN_NAN ? "NaN" : "Inf");
		return 0;
	}

	len2 += apn->_msb;

	/* Scientific notation. digits => abcd */
	int note = 0;	/* abcd */
	int ptpos = 0;	/* point position from msb */
	int zeros = 0;	/* Prepending zeros when note == 1 */
	const int adjexp = apn->_exp + apn->_msb - 1;	/* TODO: xflow */
	if (apn->_exp > 0 || adjexp < -6) {
		if (apn->_msb == 1) {
			note = 3;	/* aEadjexp */
		} else {
			note = 2;	/* a.bcdEadjexp */
			len2++;		/* "." */
			ptpos = 1;
		}
		len2 += 2;		/* "E+", "E-" */
		len2 += _tt_digits_ll(llabs(adjexp));
	} else if (apn->_exp < 0) {
		note = 1;	/* 0.0abcd, ab.cd (|exp| digits after ".") */
		len2++;		/* "." */
		zeros = abs(apn->_exp) - apn->_msb + 1;
		if (zeros > 0)
			len2 += zeros;
		else
			ptpos = -zeros + 1;
	}

	if (len2 > len)
		return TT_ENOBUFS;

	/* Prepend 0 when note == 1 */
	if (zeros > 0) {
		*str++ = '0';
		*str++ = '.';
		while (--zeros)
			*str++ = '0';
	}

	/* Get valid uints */
	int uints = (apn->_msb + 8) / 9;
	/* Point to first 3-decimals */
	int ptr = apn->_msb - (uints - 1) * 9;
	tt_assert(ptr >= 1 && ptr <= 9);
	ptr--;
	ptr /= 3;
	ptr *= 11;

	/* Generate coefficient */
	bool first = true;
	uint bit10;
	const uint *dig32 = apn->_dig32 + uints - 1;
	int digs = 0;
	while (digs < apn->_msb) {
		/* Cut 10 bits: ptr ~ ptr+9 */
		bit10 = (*dig32) >> ptr;
		bit10 &= 0x3FF;
		ptr -= 11;
		if (ptr < 0) {
			ptr = 22;
			dig32--;
		}

		/* Change to 3 decimals */
		uchar dec3[3];
		_tt_apn_to_d3(bit10, dec3);
		if (first) {
			/* Skip leading 0 */
			if (dec3[2]) {
				*str++ = dec3[2] + '0';
				if (++digs == ptpos)
					*str++ = '.';
				*str++ = dec3[1] + '0';
				if (++digs == ptpos)
					*str++ = '.';
			} else if (dec3[1]) {
				*str++ = dec3[1] + '0';
				if (++digs == ptpos)
					*str++ = '.';
			}
			first = false;
		} else {
			*str++ = dec3[2] + '0';
			if (++digs == ptpos)
				*str++ = '.';
			*str++ = dec3[1] + '0';
			if (++digs == ptpos)
				*str++ = '.';
		}
		*str++ = dec3[0] + '0';
		if (++digs == ptpos)
			*str++ = '.';
	}
	tt_assert(digs == apn->_msb);

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
