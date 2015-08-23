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

static char bin_to_ascii[] = "0123456789ABCDEF";

static int ascii_to_bin(int v)
{
	if (v >= '0' && v <= '9')
		return v - '0';
	else if (v >= 'a' && v <= 'f')
		return v - 'a' + 10;
	else if (v >= 'A' && v <= 'F')
		return v - 'A' + 10;
	else
		return -1;
}

/* Divide "dividend" by 10^9 and store quotient to "quo", return remainder
 * - "dividend" = [len-1] + [len-2]*2^31 + [len-3]*2^62 + ...
 * - Each uint in "dividend" = 0 ~ 2^31-1
 * - Low uint in "dividend" and "quo" means higher digits
 * - *len = valid uints in "dividend" on entry, valid uints in "quo" on exit
 * - "quo" is zeroed on entry
 */
static uint bin_div1b(const uint *dividend, uint *quo, uint *len)
{
	const int uints = *len;
	tt_assert_fa(uints && dividend[0]);

	int i = 0;
	uint64_t rem = 0;

	/* First quotient digit */
	while (*len) {
		rem *= BIT(31);
		rem += dividend[i];
		i++;
		if (rem >= 1000000000) {
			quo[0] = rem / 1000000000;
			rem %= 1000000000;
			break;
		}
		(*len)--;
	}

	/* Other quotient digits */
	for (int j = 1; i < uints; i++, j++) {
		uint64_t d = dividend[i];
		d += rem * BIT(31);
		quo[j] = d / 1000000000;
		rem = d % 1000000000;
	}

	return rem;
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
		int v = ascii_to_bin(*s);
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
				dig9 += ascii_to_bin(*s++);
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
			int c = ascii_to_bin(*s);
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
	if (!s) {
		tt_error("Not enough memory");
		return TT_ENOMEM;
	}
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
		uint len = ti->_msb;
		uint *dividend = malloc(len * 4);
		uint *quo = malloc(len * 4);
		char *dec = malloc(digs + 10);
		char *pd = dec + digs + 9;
		*pd-- = '\0';

		/* Lower dividend stores higher digs */
		for (uint i = 0; i < len; i++)
			dividend[i] = ti->_int[len-1-i];

		/* Divide 10^9 till quotient = 0 */
		while (len) {
			memset(quo, 0, len * 4);
			uint rem = bin_div1b(dividend, quo, &len);
			for (int i = 0; i < 9; i++) {
				*pd-- = bin_to_ascii[rem % 10];
				rem /= 10;
			}
			__tt_swap(dividend, quo);
		}
		pd++;

		/* Copy to string buffer(strip leading zeros) */
		while (*pd == '0')
			pd++;
		tt_assert(strlen(pd));
		strcpy(s, pd);

		free(dividend);
		free(quo);
		free(dec);
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
