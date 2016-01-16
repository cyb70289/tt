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
	struct tt_int ti = _TT_INT_DECL(sz, m);

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
	const char *a = "9903824712476097189547457927456428897123434496";
	const char *b = "7417779112426492492240709295800320";

	struct tt_int *tia = tt_int_alloc();
	struct tt_int *tib = tt_int_alloc();
	struct tt_int *tig = tt_int_alloc();
	struct tt_int *tiu = tt_int_alloc();
	struct tt_int *tiv = tt_int_alloc();

	tt_int_from_string(tia, a);
	tt_int_from_string(tib, b);

	tt_int_extgcd(tig, tiu, tiv, tia, tib);

	char *g = NULL, *u = NULL, *v = NULL;
	tt_int_to_string(tig, &g, 10);
	tt_int_to_string(tiu, &u, 10);
	tt_int_to_string(tiv, &v, 10);
	printf("%s = ", g);
	printf("%s * %s %s %s * %s\n", u, a, tiv->_sign?"\b":"+", v, b);

	free(g);
	free(u);
	free(v);
	tt_int_free(tia);
	tt_int_free(tib);
	tt_int_free(tig);
	tt_int_free(tiu);
	tt_int_free(tiv);
}

void test_mod(void)
{
	const char *a = "1111111111111111111111111111111111111";
	const char *b = "1267650600228229401496703205376";

	struct tt_int *tia = tt_int_alloc();
	struct tt_int *tib = tt_int_alloc();
	struct tt_int *tim = tt_int_alloc();

	tt_int_from_string(tia, a);
	tt_int_from_string(tib, b);

	tt_int_mod_inv(tim, tia, tib);

	char *m = NULL;
	tt_int_to_string(tim, &m, 10);
	printf("modinv = %s\n", m);

	free(m);
	tt_int_free(tia);
	tt_int_free(tib);
	tt_int_free(tim);
}

int main(void)
{
	tt_log_set_level(TT_LOG_WARN);

	test_gcd();
	test_mod();

	test_prime("31252509122307099513722565100727743481642064519811184448629"
		   "54305561681091773335180100000000000000000537");
	test_mersenne();
	prime_distribute();

	return 0;
}
