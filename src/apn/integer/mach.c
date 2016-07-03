/* Integer <--> Machine
 *
 * Copyright (C) 2015 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/apn/integer.h>
#include <common/lib.h>
#include "integer.h"

int tt_int_from_uint(struct tt_int *ti, uint64_t num)
{
	_tt_int_zero(ti);

	ti->buf[0] = num & ~_tt_word_top_bit;
	num >>= _tt_word_bits;
	if (num) {
		ti->buf[ti->msb++] = num & ~_tt_word_top_bit;
		num >>= _tt_word_bits;
		if (num)
			ti->buf[ti->msb++] = num;
	}

	return 0;
}

int tt_int_from_sint(struct tt_int *ti, int64_t num)
{
	if (num < 0) {
		tt_int_from_uint(ti, -num);
		ti->sign = 1;
	} else {
		tt_int_from_uint(ti, num);
	}

	return 0;
}
