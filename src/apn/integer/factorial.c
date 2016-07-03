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
	_tt_word *buf = malloc(n * 2 * _tt_word_sz);
	if (buf == NULL)
		return TT_ENOMEM;
	_tt_word *oper = buf;
	_tt_word *result = oper + n;

	int *msb = malloc(n * sizeof(int));
	if (msb == NULL) {
		free(buf);
		return TT_ENOMEM;
	}

	/* Initialize operators */
	for (int i = 0; i < n; i++) {
		result[i] = i+1;
		msb[i] = 1;
	}

	/* Divide and conqure */
	int ops = n, ret = 0;
	while (ops > 1) {
		__tt_swap(oper, result);
		memset(result, 0, n * _tt_word_sz);

		int i, j;
		_tt_word *o = oper, *r = result;

		for (i = j = 0; i < ops/2; i++, j += 2) {
			_tt_word *o2 = o + msb[j];
			msb[i] = _tt_int_mul_buf(r, o, msb[j], o2, msb[j+1]);
			if (msb[i] < 0) {
				ret = msb[i];
				goto out;
			}
			o = o2 + msb[j+1];
			r += msb[i];
		}
		if (j < ops) {
			/* ops is odd */
			memcpy(r, o, msb[j] * _tt_word_sz);
			msb[i] = msb[j];
			i++;
		}
		ops = i;

		tt_assert_fa((o-oper) <= n*_tt_word_sz);
		tt_assert_fa((r-result) <= n*_tt_word_sz);
	}
	tt_assert(ops == 1);

	/* Result in: result[], msb[0] */
	if (ti->_max < msb[0]) {
		ret = _tt_int_realloc(ti, msb[0]);
		if (ret)
			goto out;
	} else {
		_tt_int_zero(ti);
	}
	memcpy(ti->buf, result, msb[0] * _tt_word_sz);
	ti->msb = msb[0];

out:
	free(buf);
	free(msb);
	return ret;
}
