/* Factorial
 *
 * Copyright (C) 2015 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/apn/integer.h>
#include <common/lib.h>
#include "integer.h"

#include <string.h>
#include <math.h>

int tt_int_factorial(struct tt_int *ti, const int n)
{
	/* Make sure 0 < n < 2^31 */
	if (n <= 0) {
		tt_error("Invalid parameter");
		return TT_EINVAL;
	}

	/* Allocate buffers */
	uint *ibuf = malloc(n * 4 * 3);
	if (ibuf == NULL) {
		tt_error("Out of memory");
		return TT_ENOMEM;
	}
	uint *oper = ibuf;
	uint *result = ibuf + n;
	uint *msb = result + n;

	/* Initialize operators */
	for (int i = 0; i < n; i++) {
		result[i] = i+1;
		msb[i] = 1;
	}

	/* Divide and conqure */
	int ops = n;
	while (ops > 1) {
		__tt_swap(oper, result);
		memset(result, 0, n * 4);

		int i, j;
		uint *o = oper, *r = result;

		for (i = j = 0; i < ops/2; i++, j += 2) {
			uint *o2 = o + msb[j];
			msb[i] = _tt_int_mul_buf(r, o, msb[j], o2, msb[j+1]);
			o = o2 + msb[j+1];
			r += msb[i];
		}
		if (j < ops) {
			/* ops is odd */
			memcpy(r, o, msb[j] * 4);
			msb[i] = msb[j];
			i++;
		}
		ops = i;

		tt_assert_fa((o-oper) <= n*4);
		tt_assert_fa((r-result) <= n*4);
	}
	tt_assert(ops == 1);

	/* Result in: result[], msb[0] */
	if (ti->_max < msb[0]) {
		int ret = _tt_int_realloc(ti, msb[0]);
		if (ret)
			return ret;
	} else {
		_tt_int_zero(ti);
	}
	memcpy(ti->_int, result, msb[0] * 4);
	ti->_msb = msb[0];

	free(ibuf);

	return 0;
}
