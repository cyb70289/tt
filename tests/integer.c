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

void verify_add_sub(int count)
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

int main(void)
{
#if 0
	struct tt_int *ti = tt_int_alloc();

	tt_int_from_string(ti, "1267650600228229401496703205375");
	for (int i = 0; i < ti->_msb; i++)
		printf("%08x\n", ti->_int[i]);
	assert(_tt_int_sanity(ti) == 0);

	char *s = NULL;
	tt_int_to_string(ti, &s, 10);
	printf("%s\n", s);
	free(s);
	s = NULL;

	struct tt_int *ti1 = tt_int_alloc();
	struct tt_int *ti2 = tt_int_alloc();
	tt_int_from_string(ti1, "-11111111111111111111111111111111111111111111");
	tt_int_from_string(ti2, "99999999999999999999999999999");
	tt_int_sub(ti1, ti1, ti2);
	tt_int_to_string(ti1, &s, 10);
	printf("%s\n", s);
	free(s);
	s = NULL;
#endif

	srand(time(NULL));
	tt_log_set_level(TT_LOG_WARN);

	const int count = 10000;
	verify_conv(count);
	verify_add_sub(count);

	return 0;
}
