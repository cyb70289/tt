/* Common libs
 *
 * Copyright (C) 2015 Yibo Cai
 */
#include <tt/tt.h>
#include "lib.h"

#include <stdlib.h>

/* Calculate decimal digits (n >= 0) */
int _tt_digits(int n)
{
	int d = 1;
	n /= 10;

	while (n) {
		n /= 10;
		d++;
	}

	return d;
}

int _tt_digits_ll(long long int n)
{
	int d = 1;
	n /= 10;

	while (n) {
		n /= 10;
		d++;
	}

	return d;
}

int _tt_atoi(const char *str, int *i)
{
	const char *s = str;

	/* Check syntax */
	if (*s == '+' || *s == '-')
		s++;
	while (*s) {
		if (*s < '0' || *s > '9')
			return TT_EINVAL;
		s++;
	}

	/* XXX: check overflow/underflow */
	*i = atoi(str);
	return 0;
}

/* Bit reversal
 * - 1 <= bits <= 31
 * - http://www.katjaas.nl/bitreversal/bitreversal.html
 */
uint _tt_bitrev(uint n, int bits)
{
	uint rev = n;
	uint cnt = bits - 1;
	uint N = 1 << bits;

	for (n >>= 1; n; n >>= 1) {
		rev <<= 1;
		rev |= (n & 1);
		cnt--;
	}

	rev <<= cnt;
	rev &= (N - 1);

	return rev;
}
