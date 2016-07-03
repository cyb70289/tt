/* Integer <--> String
 *
 * Copyright (C) 2015 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/apn/integer.h>
#include <common/lib.h>
#include "integer.h"

#include <string.h>
#include <math.h>

#include "str-dec9.c"

static char bin_to_ascii[] = "0123456789ABCDEF";

static char ascii_to_bin[] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
	-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

/* Divide "dividend" by 10^9 and store quotient to "quo", return remainder
 * - *len = valid words in "dividend" on entry, valid words in "quo" on exit
 * - "quo" must be zeroed on entry
 */
static uint div_dec9(const _tt_word *dividend, _tt_word *quo, int *len)
{
	const int words = *len;
	const _tt_word *ddptr = dividend + words - 1;
	uint64_t rem = 0;

	/* First quotient digit */
	if (*ddptr < 1000000000) {
		rem = *ddptr;
		ddptr--;
		(*len)--;
	}

	/* Other quotient digits */
	for (int i = *len-1; i >= 0; i--) {
		_tt_word dd = *ddptr--;
#ifdef _TT_LP64_
		/* Split 63 bits into 2 uint. 64 bit division is much faster
		 * than 128 bit division.
		 */
		rem <<= 31;
		rem |= (dd >> 32);
		quo[i] = rem / 1000000000;
		rem %= 1000000000;
		quo[i] <<= 32;

		rem <<= 32;
		rem |= (uint)dd;
		quo[i] |= rem / 1000000000;
		rem %= 1000000000;
#else
		rem <<= 31;
		rem |= dd;
		quo[i] = rem / 1000000000;
		rem %= 1000000000;
#endif
	}
	return rem;
}

/* Integer to decimal conversion
 * - 9 decimal digits are packed to one uint
 * - _int, msb: integer to be converted, must have one extra word
 * - lshift: left shifted bits of _int
 * - dec buffer must be large enough
 */
static int int_to_dec(_tt_word *_int, int msb_int, int lshift,
		uint *dec, int msb_dec)
{
	if (msb_int < dec9[DEC9_CROSS_IDX].msb * 2) {
		/* Adopt classic way on small input */
		/* Shift back */
		msb_int = _tt_int_shift_buf(_int, msb_int, -lshift);

		if (msb_int == 1 && _int[0] == 0)
			return 0;

		int len = 0;
		void *workbuf = malloc(msb_int*_tt_word_sz);
		_tt_word *quo = workbuf;

		/* Divide 10^9 till quotient = 0 */
		while (msb_int) {
			memset(quo, 0, msb_int*_tt_word_sz);
			dec[len++] = div_dec9(_int, quo, &msb_int);
			__tt_swap(_int, quo);
		}
		tt_assert_fa(len <= msb_dec);

		free(workbuf);
		return 0;
	}

	/* Divide & Conquer */
	int ret = 0;

	/* Pick max radix */
	int idx = ARRAY_SIZE(dec9) - 1;
	while (idx > DEC9_CROSS_IDX) {
		if (msb_int >= dec9[idx].msb * 2)
			break;
		idx--;
	}

	/* Shift dividend to match normalized divisor */
	msb_int = _tt_int_shift_buf(_int, msb_int, dec9[idx].shift-lshift);

	/* Allocate working buffer */
	int msb_qt = msb_int - dec9[idx].msb + 2;	/* One extra word */
	int msb_rm = dec9[idx].msb + 1;			/* " */
	void *workbuf = calloc(msb_qt+msb_rm, _tt_word_sz);
	if (!workbuf)
		return TT_ENOMEM;
	_tt_word *qt = workbuf;
	_tt_word *rm = qt + msb_qt;

	/* Divide 10^n */
	ret = _tt_int_div_buf(qt, &msb_qt, rm, &msb_rm,
			_int, msb_int, dec9[idx]._int, dec9[idx].msb);
	if (ret)
		goto out;

	/* Combine */
	const int msb_dec9 = 1 << idx;
	ret = int_to_dec(rm, msb_rm, dec9[idx].shift, dec, msb_dec9);
	if (ret)
		goto out;
	ret = int_to_dec(qt, msb_qt, 0, dec+msb_dec9, msb_dec-msb_dec9);

out:
	free(workbuf);
	return ret;
}

int tt_int_from_string(struct tt_int *ti, const char *str)
{
	_tt_int_zero(ti);

	/* Check leading "+", "-" */
	if (*str == '-') {
		ti->sign = 1;
		str++;
	} else if (*str == '+') {
		str++;
	}

	/* Check radix: 0x -> 16, 0b -> 2, 0 -> 8, [1-9] -> 10 */
	int radix = 10;
	if (*str == '0') {
		radix = 8;
		str++;
		if (*str == 'x' || *str == 'X') {
			radix = 16;
			str++;
		} else if (*str == 'b' || *str == 'B') {
			radix = 2;
			str++;
		}
	}

	/* Skip leading 0 */
	while (*str == '0')
		str++;
	if (*str == '\0')
		return 0;

	/* Verify string */
	const char *s = str;
	while (*s) {
		int v = ascii_to_bin[(uchar)*s];
		if (v < 0 || v >= radix) {
			tt_debug("Invalid string");
			return TT_EINVAL;
		}
		s++;
	}

	/* Calculate digit buffer size in bits */
	const uint digs = s - str;
	uint bits;
	if (radix == 2)
		bits = digs;
	else if (radix == 8)
		bits = digs * 3;
	else if (radix == 16)
		bits = digs * 4;
	else
		bits = (uint)(((double)digs) * log2(10)) + 1;
	tt_assert(bits);

	/* Re-allocate buffer if required */
	const uint words = (bits+_tt_word_bits-1) / _tt_word_bits;
	int ret = _tt_int_realloc(ti, words);
	if (ret)
		return ret;

	/* String to Integer conversion */
	if (radix == 10) {
		s -= digs;	/* First digit */

		const uint len = (digs + 8) / 9;
		int lt = digs % 9;	/* Digits in top word */
		if (lt == 0)
			lt = 9;

		int msb = 1;	/* Valid words in product */
		_tt_word *product = ti->buf;
		_tt_word carry;
		_tt_word_double m;
		for (uint i = 0; i < len; i++) {
			/* Multiply product by 10^9 */
			carry = 0;
			for (uint j = 0; j < msb; j++) {
				m = product[j];
				m *= 1000000000;
				m += carry;
				carry = m >> _tt_word_bits;
				product[j] = m & ~_tt_word_top_bit;
			}
			if (carry)
				product[msb++] = carry;

			/* Pack 9-digits into one uint */
			uint dig9 = 0;
			for (int j = 0; j < lt; j++) {
				dig9 *= 10;
				dig9 += ascii_to_bin[(uchar)*s++];
			}
			lt = 9;

			/* Add 9 new decimal digits */
			carry = dig9;
			for (uint j = 0; j < msb; j++) {
				m = product[j];
				m += carry;
				carry = m >> _tt_word_bits;
				product[j] = m;
				if (carry)
					product[j] &= ~_tt_word_top_bit;
				else
					break;
			}
			if (carry)
				product[msb++] = carry;
		}
		ti->msb = msb;
		tt_assert(*s == '\0');
	} else {
		int cur_word = -1, cur_bit = _tt_word_bits-1;
		_tt_word mask = 0;

		s--;	/* Last digit */
		while (s >= str) {
			int c = ascii_to_bin[(uchar)*s];
			for (int i = radix; i > 1; i >>= 1) {
				cur_bit++;
				mask <<= 1;
				if (cur_bit == _tt_word_bits) {
					cur_bit = 0;
					mask = 1;
					cur_word++;
				}
				if (c & 1)
					ti->buf[cur_word] |= mask;
				c >>= 1;
			}
			s--;
		}

		ti->msb = cur_word + 1;
		/* Top int may be 0 */
		if (ti->buf[ti->msb-1] == 0)
			ti->msb--;
		tt_assert(ti->msb && ti->msb <= ti->_max);
	}

	return 0;
}

int tt_int_to_string(const struct tt_int *ti, char **str, int radix)
{
	if (*str)
		tt_warn("Possible memory leak: str != NULL");

	/* Allocate digit buffer */
	uint bits = ti->msb * _tt_word_bits, digs;
	int rbits = 0;
	const char *prefix;
	if (radix == 2) {
		digs = bits;
		rbits = 1;
		prefix = "0b";
	} else if (radix == 8) {
		digs = (bits + 2) / 3;
		rbits = 3;
		prefix = "0";
	} else if (radix == 16) {
		digs = (bits + 3) / 4;
		rbits = 4;
		prefix = "0x";
	} else if (radix == 10 || radix == 0) {
		digs = (uint)(((double)bits) * log10(2)) + 1;
		prefix = "";
	} else {
		tt_debug("Invalid radix");
		return TT_EINVAL;
	}
	char *s = calloc(digs+1+2+1, 1);	/* -, 0x, \0 */
	if (!s)
		return TT_ENOMEM;
	*str = s;

	if (ti->sign)
		*s++ = '-';

	if (_tt_int_is_zero(ti)) {
		*s = '0';
		return 0;
	}

	/* Radix prefix */
	strcat(s, prefix);
	s += strlen(prefix);

	/* Integer to string conversion */
	if (radix == 10) {
		int msb_int = ti->msb + 1;
		int msb_dec = (digs + 8) / 9 + 1;
		void *workbuf = malloc(msb_int*_tt_word_sz + msb_dec*4);
		if (!workbuf)
			return TT_ENOMEM;
		_tt_word *intbuf = workbuf;
		uint *decbuf = workbuf + msb_int*_tt_word_sz;

		memcpy(intbuf, ti->buf, ti->msb * _tt_word_sz);
		memset(decbuf, 0, msb_dec * 4);
		int err = int_to_dec(intbuf, ti->msb, 0, decbuf, msb_dec);
		if (err) {
			free(workbuf);
			return err;
		}

		int i, skip_leading_zero = 1;

		/* skip leading zero */
		for (i = msb_dec-1; i >= 0; i--) {
			if (!skip_leading_zero)
				break;

			double d = decbuf[i];
			d /= 100000000;

			for (int j = 0; j < 9; j++) {
				d += 0.000000001;
				int k = (int)d;
				d -= k;
				d *= 10;

				int c = bin_to_ascii[k];
				if (skip_leading_zero) {
					if (c != '0') {
						skip_leading_zero = 0;
						*s++ = c;
					}
				} else {
					*s++ = c;
				}
			}
		}

		/* Remaining digits */
		for (; i >= 0; i--) {
			double d = decbuf[i];
			d /= 100000000;

			for (int j = 0; j < 9; j++) {
				d += 0.000000001;
				int k = (int)d;
				d -= k;
				d *= 10;

				*s++ = bin_to_ascii[k];
			}
		}
		*s = '\0';

		free(workbuf);
	} else {
		/* Get total digits */
		int msb_bits = _tt_int_word_bits(ti->buf[ti->msb-1]);
		tt_assert(msb_bits);
		bits = (ti->msb - 1) * _tt_word_bits + msb_bits;
		digs = (bits + rbits - 1) / rbits;

		s += (digs-1);
		int cur_word = 0, cur_bit = 0, i;
		_tt_word d = ti->buf[0];
		while (digs--) {
			cur_bit += rbits;
			if (cur_bit < _tt_word_bits) {
				i = d & (radix-1);
				d >>= rbits;
			} else {
				cur_bit -= _tt_word_bits;
				i = d;
				d = ti->buf[++cur_word];
				i |= ((d << (rbits-cur_bit)) & (radix-1));
				d >>= cur_bit;
			}
			*s-- = bin_to_ascii[i];
		}
		tt_assert(cur_word == ti->msb || cur_word == ti->msb-1);
		tt_assert(*(s+1) != '0');
	}

	return 0;
}
