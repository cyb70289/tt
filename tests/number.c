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
	printf("Testing prime distribution...\n");

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

/* Random integer with size = msb */
struct tt_int *rand_int(int msb)
{
	struct tt_int *ti = tt_int_alloc();
	if (_tt_int_realloc(ti, msb))
		return NULL;

	ti->_msb = msb;
	for (int i = 0; i < msb-1; i++)
		ti->_int[i] = _tt_rand() & ~(1<<31);
	do {
		ti->_int[msb-1] = _tt_rand() & ~(1<<31);
	} while (ti->_int[msb-1] == 0);

	return ti;
}

void test_gcd(void)
{
#if 1
	printf("Testing GCD...\n");

	struct tt_int *u = tt_int_alloc();
	struct tt_int *v = tt_int_alloc();
	struct tt_int *g = tt_int_alloc();

#define GCD_MAX_MSB	100
#define GCD_COUNT	1000
	for (int i = 0; i < GCD_COUNT; i++) {
		struct tt_int *a = rand_int(_tt_rand() % GCD_MAX_MSB + 1);
		struct tt_int *b = rand_int(_tt_rand() % GCD_MAX_MSB + 1);

		tt_int_extgcd(g, u, v, a, b);

		tt_int_mul(u, a, u);
		tt_int_mul(v, b, v);
		tt_int_add(u, u, v);

		if (tt_int_cmp(g, u)) {
			tt_error("GCD test failed!");
			_tt_int_print(a);
			_tt_int_print(b);
			break;
		}

		tt_int_free(a);
		tt_int_free(b);
	}

	tt_int_free(u);
	tt_int_free(v);
	tt_int_free(g);

	printf("Done\n");
#else
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
#endif
}

/* Test montgomery reduce */
void test_mont(int nl)
{
	struct tt_int *n = rand_int(nl);
	n->_int[0] |= 1;

	int l = _tt_rand() % nl;
	if (l == 0)
		l = 1;
	struct tt_int *a = rand_int(l);
	l = _tt_rand() % nl;
	if (l == 0)
		l = 1;
	struct tt_int *b = rand_int(l);

	/* lambda = (2^31)^(n->_msb) */
	struct tt_int *lambda = tt_int_alloc();
	_tt_int_realloc(lambda, n->_msb+1);
	lambda->_int[n->_msb] = 1;
	lambda->_msb = n->_msb+1;

	/* u = -1/n % lambda */
	struct tt_int *u = tt_int_alloc();
	tt_int_mod_inv(u, n, lambda);
	if (u->_sign == 0)
		tt_int_sub(u, lambda, u);

	/* c = (a * b * lambda) % n */
	struct tt_int *c = tt_int_alloc();
	tt_int_mul(c, a, b);
	tt_int_mul(c, c, lambda);
	tt_int_div(NULL, c, c, n);

	/* a = (a * lambda) % n, b = (b * lambda) % n */
	tt_int_mul(a, a, lambda);
	tt_int_div(NULL, a, a, n);
	tt_int_mul(b, b, lambda);
	tt_int_div(NULL, b, b, n);

	/* d = montgomery(a * b) */
	struct tt_int *d = tt_int_alloc();
	tt_int_mul(d, a, b);
	_tt_int_realloc(d, n->_msb*2+1);

	uint *t = malloc((n->_msb+1)*3*4);
	_tt_int_mont_reduce(d->_int, &d->_msb, d->_int, d->_msb,
			u->_int, u->_msb, n->_int, n->_msb, t);

	/* c == d? */
	if (_tt_int_cmp_buf(c->_int, c->_msb, d->_int, d->_msb))
		tt_error("Montgomery failed!");

	free(t);
	tt_int_free(a);
	tt_int_free(b);
	tt_int_free(n);
	tt_int_free(lambda);
	tt_int_free(u);
	tt_int_free(c);
	tt_int_free(d);
}

int main(void)
{
	tt_log_set_level(TT_LOG_WARN);

#if 0
	test_prime("31252509122307099513722565100727743481642064519811184448629"
		   "54305561681091773335180100000000000000000537");
	test_mersenne();
	for (int i = 1; i < 3000; i++)
		test_mont(i % 300 + 1);
#endif
	test_gcd();
	prime_distribute();

	return 0;
}
