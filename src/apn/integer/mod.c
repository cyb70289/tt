/* Modular
 *
 * Copyright (C) 2016 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/apn/integer.h>
#include <common/lib.h>
#include "integer.h"

/* Modular inverse
 * - m = 1/a mod b
 * - a, b must be coprime
 */
int tt_int_mod_inv(struct tt_int *m,
		const struct tt_int *a, const struct tt_int *b)
{
	struct tt_int *n = tt_int_alloc();

	int ret = tt_int_extgcd(NULL, m, n, a, b);
	tt_int_free(n);

	return ret;
}
