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

	tt_int_from_string(ti, "1267650600228229401496703205375");
	for (int i = 0; i < ti->_msb; i++)
		printf("%08x\n", ti->_int[i]);

	char *s = NULL;
	tt_int_to_string(ti, &s, 16);
	printf("%s\n", s);
	free(s);

	return 0;
}
