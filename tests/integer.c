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
#include <assert.h>

#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

#define MAX_DIGS	5000

static const char bin2asc[] = "0123456789ABCDEF";

/* Generate random integer string */
static char *gen_int_str(int *radix)
{
	const int _radix[] = { 2, 8, 10, 16, 10 };
	const char *prefix[] = { "0b", "0", "", "0x", "" };

	const int sign = rand() & 1;
	const int ridx = rand() % 5;
	const int digs = rand() % MAX_DIGS + 1;

	char *str = malloc(sign + strlen(prefix[ridx]) + digs + 1);
	char *s = str;
	if (sign)
		*s++ = '-';
	*s = '\0';
	strcat(s, prefix[ridx]);
	s += strlen(prefix[ridx]);

	*s = bin2asc[rand() % _radix[ridx]];
	if (*s == '0')
		*s = '1';
	s++;
	for (int i = 1; i < digs; i++)
		*s++ = bin2asc[rand() % _radix[ridx]];
	*s = '\0';

	*radix = _radix[ridx];
	return str;
}

static void verify_conv(int count)
{
	printf("Convert...\n");

	struct tt_int *ti = tt_int_alloc();
	char *s = NULL;
	int ret;

	/* Special case */
	const struct {
		const char *str;
		int radix;
		const char *_int;
	} cases[] = {
		{ "0", 10, "0", },
		{ "0000000000000000000000000000000000", 16, "0", },
		{ "-000000000000000000000000000000000", 8, "-0", },
		{ "0x000000000000000000000000000", 2, "0", },
		{ "000006", 16, "0x6", },
		{ "-0x000F", 8, "-017", },
	};

	for (int i = 0; i < sizeof(cases)/sizeof(cases[0]); i++) {
		ret = tt_int_from_string(ti, cases[i].str);
		assert(ret == 0 && _tt_int_sanity(ti) == 0);

		ret = tt_int_to_string(ti, &s, cases[i].radix);
		assert(ret == 0);
		if (strcmp(s, cases[i]._int)) {
			tt_error("%s => %s (%s)",
					cases[i].str, s, cases[i]._int);
			return;
		}
		free(s);
		s = NULL;
	}

	while (count--) {
		int radix;
		char *str = gen_int_str(&radix);

		ret = tt_int_from_string(ti, str);
		assert(ret == 0 && _tt_int_sanity(ti) == 0);
		ret = tt_int_to_string(ti, &s, radix);
		assert(ret == 0);

		if (strcmp(s, str)) {
			tt_error("%s => %s", str, s);
			break;
		}

		free(str);
		free(s);
		s = NULL;
	}

	tt_int_free(ti);
}

static void verify_add_sub(int count)
{
	printf("Add & Sub...\n");

	struct tt_int *a = tt_int_alloc();
	struct tt_int *b = tt_int_alloc();
	struct tt_int *a2 = tt_int_alloc();
	struct tt_int *b2 = tt_int_alloc();
	struct tt_int *t = tt_int_alloc();

	for (int i = 1; i <= count; i++) {
		if (i % 100 == 0) {
			tt_int_free(a);
			tt_int_free(b);
			tt_int_free(a2);
			tt_int_free(b2);
			tt_int_free(t);

			a = tt_int_alloc();
			b = tt_int_alloc();
			a2 = tt_int_alloc();
			b2 = tt_int_alloc();
			t = tt_int_alloc();
		}

		int ret, radix;
		char *str;

		/* Generate a, b */
		str = gen_int_str(&radix);
		tt_int_from_string(a, str);
		free(str);
		str = gen_int_str(&radix);
		tt_int_from_string(b, str);
		free(str);

		/* a2 = a+a, b2 = b+b */
		ret = tt_int_add(a2, a, a);
		assert(ret == 0 && _tt_int_sanity(a2) == 0);
		ret = tt_int_add(b2, b, b);
		assert(ret == 0 && _tt_int_sanity(b2) == 0);

		/* t = a+b */
		ret = tt_int_add(t, a, b);
		assert(ret == 0 && _tt_int_sanity(t) == 0);

		/* a = a-b */
		ret = tt_int_sub(a, a, b);
		assert(ret == 0 && _tt_int_sanity(a) == 0);

		/* b = t-a ==> b2 */
		ret = tt_int_sub(b, t, a);
		assert(ret == 0 && _tt_int_sanity(b) == 0);

		/* a = t+a ==> a2 */
		ret = tt_int_add(a, t, a);
		assert(ret == 0 && _tt_int_sanity(a) == 0);

		if (tt_int_cmp(a, a2) || tt_int_cmp(b, b2)) {
			tt_error("mismatch");
			break;
		}
	}

	tt_int_free(a);
	tt_int_free(b);
	tt_int_free(a2);
	tt_int_free(b2);
	tt_int_free(t);
}

static void verify_mul_div(int count)
{
	printf("Mul & Div...\n");

	struct tt_int *a = tt_int_alloc();
	struct tt_int *b = tt_int_alloc();
	struct tt_int *q = tt_int_alloc();
	struct tt_int *r = tt_int_alloc();

	for (int i = 1; i <= count; i++) {
		if (i % 50 == 0) {
			tt_int_free(a);
			tt_int_free(b);
			tt_int_free(q);
			tt_int_free(r);

			a = tt_int_alloc();
			b = tt_int_alloc();
			q = tt_int_alloc();
			r = tt_int_alloc();
		}

		int ret, radix;
		char *str;

		/* Generate a, b */
		str = gen_int_str(&radix);
		tt_int_from_string(a, str);
		free(str);
		str = gen_int_str(&radix);
		tt_int_from_string(b, str);
		free(str);

		if (a->msb < b->msb) {
			void *t = a;
			a = b;
			b = t;
		}

		/* q = a / b, r = a % b */
#if 1
		ret = tt_int_div(q, r, a, b);
#else
		ret = tt_int_div(NULL, r, a, b);
		ret = tt_int_div(q, NULL, a, b);
#endif
		assert(ret == 0 && _tt_int_sanity(q) == 0 &&
				_tt_int_sanity(r) == 0);

		/* b = q * b + r */
		ret = tt_int_mul(b, q, b);
		assert(ret == 0 && _tt_int_sanity(b) == 0);
		ret = tt_int_add(b, b, r);

		/* a == b */
		if (tt_int_cmp(a, b)) {
			tt_error("mismatch");
			break;
		}
	}

	tt_int_free(a);
	tt_int_free(b);
	tt_int_free(q);
	tt_int_free(r);
}

struct tt_int *rand_int(int msb)
{
	struct tt_int *ti = tt_int_alloc();
	if (_tt_int_realloc(ti, msb))
		return NULL;

	ti->msb = msb;
#ifdef _TT_LP64_
	for (int i = 0; i < msb-1; i++) {
		ti->buf[i] = rand() & ~(1<<31);
		ti->buf[i] <<= 32;
		ti->buf[i] |= rand();
	}
	do {
		ti->buf[msb-1] = rand() & ~(1<<31);
	} while (ti->buf[msb-1] == 0);
	ti->buf[msb-1] <<= 32;
	ti->buf[msb-1] |= rand();
#else
	for (int i = 0; i < msb-1; i++)
		ti->buf[i] = rand() & ~(1<<31);
	do {
		ti->buf[msb-1] = rand() & ~(1<<31);
	} while (ti->buf[msb-1] == 0);
#endif

	return ti;
}

void gen_exp10(int e)
{
	char *s = malloc(e+2);
	memset(s+1, '0', e);
	s[0] = '1';
	s[e+1] = '\0';

	struct tt_int *ti = tt_int_alloc();
	tt_int_from_string(ti, s);

	int c = 0;
	_tt_word sh = 0;
	const int shift = _tt_word_bits - _tt_int_word_bits(ti->buf[ti->msb-1]);
	printf("#define dec9_shift_%d\t%d\n", e/9, shift);

#ifdef _TT_LP64_
	printf("static const uint64_t dec9_norm_%d[] = {", e/9);
	for (int i = 0; i < ti->msb; i++) {
		if (c == 0)
			printf("\n\t");
		printf("0x%016llX,",
				((ti->buf[i] << shift) | sh) & ~(1ULL << 63));
		sh = ti->buf[i] >> (63 - shift);
		c++;
		if (c == 3)
			c = 0;
		else
			printf(" ");
	}
#else
	printf("static const uint dec9_norm_%d[] = {", e/9);
	for (int i = 0; i < ti->msb; i++) {
		if (c == 0)
			printf("\n\t");
		printf("0x%08X,", ((ti->buf[i] << shift) | sh) & ~(1 << 31));
		sh = ti->buf[i] >> (31 - shift);
		c++;
		if (c == 6)
			c = 0;
		else
			printf(" ");
	}
#endif
	printf("\n};\n\n");
}

int main(void)
{
	srand(time(NULL));

#if 0
	char *s = NULL;
	struct tt_int *ti = tt_int_alloc();
	struct tt_int *ti1 = tt_int_alloc();
	struct tt_int *ti2 = tt_int_alloc();
	tt_int_from_string(ti1, "-1111111111111111111111111111111111111111111");
	tt_int_from_string(ti2, "99999999999999999999999999999");
	tt_int_sub(ti1, ti1, ti2);
	tt_int_to_string(ti1, &s, 10);
	printf("%s\n", s);
	free(s);
	s = NULL;

	tt_int_from_string(ti1, "0x1000000000000000000000000000000000000000");
	tt_int_from_string(ti2, "0x20000000000000000000000000000000000");
	tt_int_mul(ti, ti1, ti2);
	assert(_tt_int_sanity(ti) == 0);
	tt_int_to_string(ti, &s, 16);
	printf("%s\n", s);
	free(s);
	s = NULL;

	tt_int_from_string(ti1, "1111111111111111111111111111111111111111111");
	tt_int_from_string(ti2, "1111111111111111111111111111");
	tt_int_div(ti, ti1, ti1, ti2);
	assert(_tt_int_sanity(ti) == 0);
	assert(_tt_int_sanity(ti1) == 0);
	tt_int_to_string(ti, &s, 10);
	printf("%s\n", s);
	free(s);
	s = NULL;
	tt_int_to_string(ti1, &s, 10);
	printf("%s\n", s);
	free(s);
	s = NULL;

	tt_int_free(ti);
	tt_int_free(ti1);
	tt_int_free(ti2);

	return 0;
#endif

#if 0
	int old_level = tt_log_set_level(TT_LOG_INFO);
	tt_info("Calculating...");
	struct tt_int *ti = tt_int_alloc();
	tt_int_factorial(ti, 1000000);
	char *s = NULL;
	tt_info("Converting...");
	tt_int_to_string(ti, &s, 10);
	printf("%s\n", s);
	free(s);
	tt_int_free(ti);
	tt_log_set_level(old_level);
	return 0;
#endif

#if 0
	#define MSB1	3000000
	#define MSB2	300
	#define LOOPS	1

	tt_log_set_level(TT_LOG_WARN);

	struct tt_int *ti1 = rand_int(MSB1);
	struct tt_int *ti2 = rand_int(MSB2);
	struct tt_int *ti3 = tt_int_alloc();

	for (int i = 0; i < LOOPS; i++)
		tt_int_mul(ti3, ti1, ti2);

	return 0;
#endif

#if 0
	tt_log_set_level(TT_LOG_WARN);
	for (int i = 1; i <= 4096; i *= 2)
		gen_exp10(i*9);
	return 0;
#endif

	tt_log_set_level(TT_LOG_WARN);

	const int count = 10000;
	verify_conv(count);
	verify_add_sub(count);
	verify_mul_div(count);

	return 0;
}
