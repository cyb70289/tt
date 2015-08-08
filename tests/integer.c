/* Test arbitrary precision integer
 *
 * Copyright (C) 2015 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/apn/integer.h>
#include <apn/integer/integer.h>

#include <math.h>
#include <time.h>
#include <string.h>

#pragma GCC diagnostic ignored "-Wunused-function"

int main(void)
{
	struct tt_int *ti = tt_int_alloc();

	tt_int_from_string(ti, "123456789012345678901234567890");
	for (int i = 0; i < ti->_msb; i++)
		printf("%08x\n", ti->_int[i]);

	return 0;
}
