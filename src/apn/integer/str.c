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
 * - *len = valid uints in "dividend" on entry, valid uints in "quo" on exit
 * - "quo" is zeroed on entry
 */
static uint div_dec9(const uint *dividend, uint *quo, int *len)
{
	const int uints = *len;

	const uint *ddptr = dividend + uints - 1;
	uint64_t rem = 0;

	/* First quotient digit */
	if (*ddptr < 1000000000) {
		rem = *ddptr;
		ddptr--;
		(*len)--;
	}

	/* Other quotient digits */
	for (int i = *len-1; i >= 0; i--) {
		rem *= BIT(31);
		rem += *ddptr--;
		quo[i] = rem / 1000000000;
		rem %= 1000000000;
	}

	return rem;
}

/* Integer to decimal conversion
 * - 9 decimal digits are packed to one uint
 * - _int, msb: integer to be converted, must have one extra word
 * - dec buffer must be large enough
 */
static int int_to_dec(uint *_int, int msb_int, uint *dec, int msb_dec)
{
	/* Adopt classic way on small input */
	if (msb_int < dec9[DEC9_CROSS_IDX].msb * 2) {
		if (msb_int == 1 && _int[0] == 0)
			return 0;

		int len = 0;
		uint *workbuf = malloc(msb_int * 4);
		uint *quo = workbuf;

		/* Divide 10^9 till quotient = 0 */
		while (msb_int) {
			memset(quo, 0, msb_int * 4);
			dec[len++] = div_dec9(_int, quo, &msb_int);
			__tt_swap(_int, quo);
		}
		tt_assert_fa(len <= msb_dec);

		free(workbuf);
		return 0;
	}

	/* Adopt divide & conquer approach */
	int ret = 0;

	/* Pick max radix */
	int idx = ARRAY_SIZE(dec9) - 1;
	while (idx > DEC9_CROSS_IDX) {
		if (msb_int >= dec9[idx].msb * 2)
			break;
		idx--;
	}

	/* Shift dividend to match normalized divisor */
	const int dec9_shift = dec9[idx].shift;
	if (dec9_shift) {
		uint tmp = 0, tmp2;
		for (int i = 0; i < msb_int; i++) {
			tmp2 = _int[i];
			_int[i] = ((_int[i] << dec9_shift) | tmp) & ~BIT(31);
			tmp = tmp2 >> (31 - dec9_shift);
		}
		if (tmp)
			_int[msb_int++] = tmp;
	}

	/* Allocate working buffer */
	int msb_qt = msb_int - dec9[idx].msb + 2;	/* One extra word */
	int msb_rm = dec9[idx].msb + 1;			/* " */
	void *workbuf = calloc(msb_qt + msb_rm, 4);
	if (!workbuf)
		return TT_ENOMEM;
	uint *qt = workbuf;
	uint *rm = qt + msb_qt;

	/* Divide 10^n */
	ret = _tt_int_div_buf(qt, &msb_qt, rm, &msb_rm,
			_int, msb_int, dec9[idx]._int, dec9[idx].msb);
	if (ret)
		goto out;

	/* Shift remainder back */
	if (dec9_shift) {
		uint tmp = 0, tmp2;
		for (int i = msb_rm-1; i >= 0; i--) {
			tmp2 = rm[i];
			rm[i] = (rm[i] >> dec9_shift) | tmp;
			tmp = (tmp2 << (31 - dec9_shift)) & ~BIT(31);
		}
		if (rm[msb_rm-1] == 0 && msb_rm > 1)
			msb_rm--;
	}

	/* Combine */
	const int msb_dec9 = 1 << idx;
	ret = int_to_dec(rm, msb_rm, dec, msb_dec9);
	if (ret)
		goto out;
	ret = int_to_dec(qt, msb_qt, dec + msb_dec9, msb_dec - msb_dec9);

out:
	free(workbuf);
	return ret;
}

int tt_int_from_string(struct tt_int *ti, const char *str)
{
	_tt_int_zero(ti);

	/* Check leading "+", "-" */
	if (*str == '-') {
		ti->_sign = 1;
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
	const uint uints = (bits + 30) / 31;
	int ret = _tt_int_realloc(ti, uints);
	if (ret)
		return ret;

	/* String to Integer conversion */
	if (radix == 10) {
		s -= digs;	/* First digit */

		const uint len = (digs + 8) / 9;
		int lt = digs % 9;	/* Digits in top uint */
		if (lt == 0)
			lt = 9;

		uint msb = 1;	/* Valid uints in product */
		uint *product = ti->_int;
		uint carry;
		uint64_t m;
		for (uint i = 0; i < len; i++) {
			/* Multiply product by 10^9 */
			carry = 0;
			for (uint j = 0; j < msb; j++) {
				m = product[j] * 1000000000ULL;
				m += carry;
				carry = m >> 31;
				if (carry)
					product[j] = m & (BIT(31)-1);
				else
					product[j] = m;
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
				m = product[j] + carry;
				carry = m >> 31;
				if (carry) {
					product[j] = m & (BIT(31)-1);
				} else {
					product[j] = m;
					break;
				}
			}
			if (carry)
				product[msb++] = carry;
		}
		ti->_msb = msb;
		tt_assert(*s == '\0');
	} else {
		int cur_int = -1, cur_bit = 30;

		s--;	/* Last digit */
		while (s >= str) {
			int c = ascii_to_bin[(uchar)*s];
			for (int i = radix; i > 1; i >>= 1) {
				cur_bit++;
				if (cur_bit == 31) {
					cur_bit = 0;
					cur_int++;
				}
				if ((c & 1))
					ti->_int[cur_int] |= BIT(cur_bit);
				c >>= 1;
			}
			s--;
		}

		ti->_msb = cur_int + 1;
		tt_assert(ti->_msb && ti->_msb <= ti->_max);
		/* Top int may be 0 */
		if (ti->_int[ti->_msb-1] == 0) {
			ti->_msb--;
			tt_assert(ti->_msb);
		}
	}

	return 0;
}

int tt_int_to_string(const struct tt_int *ti, char **str, int radix)
{
	if (*str)
		tt_warn("Possible memory leak: str != NULL");

	/* Allocate digit buffer (in 31 bits granularity) */
	uint bits = ti->_msb * 31, digs;
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

	if (ti->_sign)
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
		int msb_int = ti->_msb + 1;
		int msb_dec = (digs + 8) / 9 + 1;
		void *workbuf = calloc(msb_int + msb_dec, 4);
		if (!workbuf)
			return TT_ENOMEM;
		uint *intbuf = workbuf;
		uint *decbuf = intbuf + msb_int;

		memcpy(intbuf, ti->_int, ti->_msb * 4);
		int err = int_to_dec(intbuf, ti->_msb, decbuf, msb_dec);
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
		int i;
		uint d = ti->_int[ti->_msb-1];
		for (i = 30; i >= 0; i--) {
			if ((d & BIT(30)))
				break;
			d <<= 1;
		}
		tt_assert(i >= 0);
		bits = (ti->_msb - 1) * 31 + i + 1;
		digs = (bits + rbits - 1) / rbits;

		s += (digs-1);
		uint cur_int = 0, cur_bit = 0;
		d = ti->_int[0];
		while (digs--) {
			cur_bit += rbits;
			if (cur_bit < 31) {
				i = d & (radix-1);
				d >>= rbits;
			} else {
				cur_bit -= 31;
				i = d;
				d = ti->_int[++cur_int];
				i |= ((d << (rbits-cur_bit)) & (radix-1));
				d >>= cur_bit;
			}
			*s-- = bin_to_ascii[i];
		}
		tt_assert(cur_int == ti->_msb || cur_int == ti->_msb-1);
		tt_assert(*(s+1) != '0');
	}

	return 0;
}
