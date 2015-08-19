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
	tt_assert(ti->_max >= 3);

	_tt_int_zero(ti);

	ti->_int[0] = num & (BIT(31)-1);
	num >>= 31;
	if (num) {
		ti->_int[ti->_msb++] = num & (BIT(31)-1);
		num >>= 31;
		if (num)
			ti->_int[ti->_msb++] = num;
	}

	return 0;
}

int tt_int_from_sint(struct tt_int *ti, int64_t num)
{
	if (num < 0) {
		tt_int_from_uint(ti, -num);
		ti->_sign = 1;
	} else {
		tt_int_from_uint(ti, num);
	}

	return 0;
}
