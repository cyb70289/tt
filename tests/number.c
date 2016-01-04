/* Number: gcd, prime
 *
 * Copyright (C) 2015 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/apn/integer.h>
#include <apn/integer/integer.h>
#include <common/lib.h>

#include <string.h>
#include <math.h>

void test_prime(const char *s)
{
	struct tt_int *ti = tt_int_alloc();
	tt_int_from_string(ti, s);

	if (tt_int_isprime(ti))
		printf("Prime: ");
	else
		printf("Composite: ");
	printf("%s\n", s);

	tt_int_free(ti);
}

void nth_prime(uint n)
{
	printf("%d-th prime: ", n);

	if (n == 1) {
		printf("2\n");
		return;
	}

	struct tt_int *ti = tt_int_alloc();
	struct tt_int *two = tt_int_alloc();
	tt_int_from_uint(ti, 1);
	tt_int_from_uint(two, 2);

	while (--n) {
		tt_int_add(ti, ti, two);
		while (!tt_int_isprime(ti))
			tt_int_add(ti, ti, two);
	}

	char *s = NULL;
	tt_int_to_string(ti, &s, 10);
	printf("%s\n", s);

	free(s);
	tt_int_free(ti);
	tt_int_free(two);
}

void test_mersenne(void)
{
	/* Mersenne number: 2^N - 1 */
	#define N	521
	const int n = N + 1;
	int sz = (n + 30) / 31;
	uint m[sz+1];
	m[sz] = 0;
	for (int i = 0; i < sz-1; i++)
		m[i] = 0x7FFFFFFF;
	m[sz-1] = (1 << (n - 31*(sz-1) - 1)) - 1;
	if (m[sz-1] == 0)
		sz--;
	struct tt_int ti = {
		._sign = 0,
		.__sz = sz,
		._max = sz,
		._msb = sz,
		._int = m,
	};

	if (tt_int_isprime(&ti))
		printf("Prime: ");
	else
		printf("Composite: ");
	printf("2^%d - 1\n", N);
}

void prime_distribute(void)
{
	#define NN	8

	uint r[NN];
	struct tt_int ti = {
		._sign = 0,
		.__sz = NN,
		._max = NN,
		._int = r,
	};

	/* In theory, there should be 100 primes in pcnt100 tries  */
	uint pcnt100 = (uint)(log(2.0) * 31.0 * NN * 100.0);
	uint pcnt = 0;

	for (int i = 0; i < pcnt100; i++) {
		/* Generate random integer with 1 ~ N words */
		for (int j = 0; j < NN; j++)
			r[j] = _tt_rand() & ~BIT(31);
		ti._msb = _tt_int_get_msb(r, NN);

		if (tt_int_isprime(&ti))
			pcnt++;
	}

	printf("%d primes found\n", pcnt);
}

void test_gcd(void)
{
	const char *a = "57715475943958356502146146744435122067445196";
	const char *b = "171466873366445888634645110566534137709077062";

	struct tt_int *ti1 = tt_int_alloc();
	struct tt_int *ti2 = tt_int_alloc();
	struct tt_int *ti3 = tt_int_alloc();

	tt_int_from_string(ti1, a);
	tt_int_from_string(ti2, b);

	tt_int_gcd(ti3, ti1, ti2);

	char *g = NULL;
	tt_int_to_string(ti3, &g, 10);
	printf("%s\n", g);
}

int main(void)
{
	tt_log_set_level(TT_LOG_WARN);

	test_gcd();

#if 0
	test_prime("31252509122307099513722565100727743481642064519811184448629"
		   "54305561681091773335180100000000000000000537");
	test_mersenne();
	prime_distribute();
#endif

	return 0;
}
