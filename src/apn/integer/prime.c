/* Primary testing
 *
 * Copyright (C) 2015 Yibo Cai
 */

#include <tt/tt.h>
#include <tt/apn/integer.h>
#include <common/lib.h>
#include "integer.h"

/* Naive algorithm (p < 2^31) */
static int naive(uint p)
{
	if (p == 2 || p == 3)
		return 1;
	if (p < 2 || p % 2 == 0 || p % 3 == 0)
		return 0;

	uint i = 5;
	while (i*i <= p) {
		if (p % i == 0 || p % (i+2) == 0)
			return 0;
		i += 6;
	}

	return 1;
}

/* Miller-Rabin algorithm */
static int miller_rabin(const uint *p, int msb)
{
	/* TODO */
	return 0;
}

/* Probability prime testing */
int tt_int_isprime(const struct tt_int *ti)
{
	if (ti->_msb == 1)
		return naive(ti->_int[0]);
	else
		return miller_rabin(ti->_int, ti->_msb);
}
