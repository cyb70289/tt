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

static int ascii_to_bin(int v)
{
	if (v >= '0' && v <= '9')
		v = v - '0';
	else if (v >= 'a' && v <= 'f')
		v = v - 'a' + 10;
	else if (v >= 'A' && v <= 'F')
		v = v - 'A' + 10;
	else
		v = -1;
	return v;
}

/* Divide "bcd" by 2^31 and store quotient to "quo"
 * - "bcd" and "quo" are BCD string: 0 ~ 9
 * - Return remainder in binary format
 * - *len = valid digs in "bcd" on enter, = valid digs in "quo" on exit
 * - "quo" is zeroed on entry
 */
static uint bcd_div2g(const char *bcd, char *quo, uint *len)
{
	const int digs = *len;
	tt_assert_fa(digs && bcd[0]);

	int i = 0;
	uint64_t rem = 0;

	/* First quotient digit */
	while (*len) {
		rem *= 10;
		rem += bcd[i++];
		if (rem >= BIT(31)) {
			quo[0] = rem / BIT(31);
			rem %= BIT(31);
			break;
		}
		(*len)--;
	}

	/* Other quotient digits */
	for (int j = 1; i < digs; i++, j++) {
		uint64_t d = bcd[i];
		d += rem * 10;
		quo[j] = d / BIT(31);
		rem = d % BIT(31);
	}

	return rem;
}

int tt_int_from_string(struct tt_int *ti, const char *str)
{
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
	if (*str == '\0') {
		_tt_int_zero(ti);
		return 0;
	}

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
	uint uints = (bits + 30) / 31;
	if (uints > ti->_cnt) {
		/* Double current buffer */
		if (uints < ti->_cnt * 2)
			uints = ti->_cnt * 2;

		uint *_int = realloc(ti->_int, uints * 4);
		if (!_int) {
			tt_error("Not enough memory");
			return TT_ENOMEM;
		}
		ti->_cnt = uints;
		tt_debug("Integer reallocated: %u bytes", uints * 4);
	}
	_tt_int_zero(ti);

	/* String to Integer conversion
	 * - Decimal: Base 2^31 division to achieve fast conversion.
	 * - Binary:  Bit by bit. Should we improve it?
	 */
	int cur_int = -1, cur_bit = 30;
	if (radix == 10) {
		/* Decimal */
		uint len = digs;
		char *bcd = malloc(digs);
		char *quo = malloc(digs);
		for (int i = 0; i < digs; i++)
			bcd[i] = ascii_to_bin(str[i]);
		while (len) {
			memset(quo, 0, digs);
			ti->_int[++cur_int] = bcd_div2g(bcd, quo, &len);
			__tt_swap(bcd, quo);
		}
		free(bcd);
		free(quo);
	} else {
		/* Binary */
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
	}

	ti->_msb = cur_int + 1;
	tt_assert(ti->_msb && ti->_msb <= ti->_cnt);
	/* Top int may be 0 */
	if (ti->_int[ti->_msb-1] == 0) {
		ti->_msb--;
		tt_assert(ti->_msb);
	}

	return 0;
}
