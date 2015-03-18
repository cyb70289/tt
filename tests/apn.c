/* Test arbitrary precision number
 *
 * Copyright (C) 2015 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/apn/apn.h>
#include <apn/apn.h>

#include <math.h>
#include <time.h>
#include <string.h>

#pragma GCC diagnostic ignored "-Wunused-function"

#ifdef __STDC_IEC_559__
/* Generate float within 2E-mexp ~ 2E+mexpr
 * - Directly fill IEEE-754 bits
 */
static double gen_float(int mexp)
{
	union {
		uint64_t i;
		double d;
	} v = {
		.i = 0,
	};

	/* Sign: 0,1 */
	if ((rand() & 1))
		v.i |= 1ULL << 63;

	/* Exponent: -mexp ~ +mexp */
	double _exp = (double)(rand() % (mexp + 1));	/* 0 ~ mexp */
	if ((rand() & 1))
		_exp = -_exp;
	int e = 1023;
	if (_exp > 0)
		e = (int)(_exp / log10(2) + 1023);
	else if (_exp < 0)
		e = (int)(_exp / log10(2) + 1024);
	v.i |= ((int64_t)e) << 52;

	/* Mantissa */
	int64_t i64 = rand();
	i64 *= rand();
	v.i |= (i64 & ((1ULL << 52) - 1));

	return v.d;
}
#endif

static bool deq(double d1, double d2)
{
	if (d1 == 0)
		return d2 == 0;
	long double dd = (long double)d1 - (long double)d2;
	dd /= d1;
	return dd < 1E-15;
}

static long double get_num(char *s, int lmin, int lmax)
{
	long double ld = 0;

	/* len: lmin ~ lmax */
	int len = lmin + rand() % (lmax - lmin + 1);
	for (int i = 0; i < len; i++) {
		s[i] = rand() % 10;
		ld *= 10;
		ld += s[i];
		s[i] += '0';
	}
	s[len] = '\0';

	return ld;
}

static void verify_add(int count)
{
	bool fail = false;
	char s[1024];
	struct tt_apn *apn1, *apn2, *apn3;

	int old_level = tt_log_set_level(TT_LOG_ERROR);

	/* Special case */
	apn1 = tt_apn_alloc(0);
	apn2 = tt_apn_alloc(0);
	apn3 = tt_apn_alloc(0);
	static const struct {
		const char *add1, *add2, *result;
	} cases[] = {
		{ "0", "0", "0", },
		{ "0E-10", "0", "0E-10" },
		{ "0E-2000", "0", "0E-2000" },
		{ "0E-20", "0E-10", "0E-20" },
		{ "0E-20", "0E-1000", "0E-1000" },
	};
	for (int i = 0; i < sizeof(cases)/sizeof(cases[0]); i++) {
		tt_apn_from_string(apn1, cases[i].add1);
		tt_apn_from_string(apn2, cases[i].add2);
		tt_apn_add(apn3, apn1, apn2);
		tt_apn_to_string(apn3, s, 360);
		if (_tt_apn_sanity(apn3)) {
			tt_error("Sanity error");
			fail = true;
			break;
		}
		if (strcmp(s, cases[i].result)) {
			tt_error("%s + %s => %s",
					cases[i].add1, cases[i].add2, s);
			fail = true;
			break;
		}
	}
	tt_apn_free(apn1);
	tt_apn_free(apn2);
	tt_apn_free(apn3);
	if (fail)
		return;

	/* Big number */
	printf("Testing big number...\n");
	apn1 = tt_apn_alloc(30);
	apn2 = tt_apn_alloc(50);
	apn3 = tt_apn_alloc(40);
	for (int i = 0; i < count; i++) {
		double d1, d2;
		long double ld;
		char s1[128], s2[128];
		ld = get_num(s1, 1, 100);
		tt_apn_from_string(apn1, s1);
		ld += get_num(s2, 1, 100);
		tt_apn_from_string(apn2, s2);
		tt_apn_add(apn3, apn2, apn1);
		tt_apn_add(apn2, apn1, apn2);
		tt_apn_to_float(apn3, &d1);
		tt_apn_to_float(apn2, &d2);

		if (_tt_apn_sanity(apn1) || _tt_apn_sanity(apn2) ||
				_tt_apn_sanity(apn3)) {
			tt_error("Sanity error");
			fail = true;
			break;
		}

		if (!deq(ld, d1) || !deq(ld, d2)) {
			tt_error("Num add error: %s + %s\nCorrect: %.18LE\n"
					"Error:   %.18E\nError:   %.18E ",
					s1, s2, ld, d1, d2);
			fail = true;
			break;
		}
	}

	tt_apn_free(apn1);
	tt_apn_free(apn2);
	tt_apn_free(apn3);
	if (fail)
		return;

#ifdef __STDC_IEC_559__
	/* Random float */
	printf("Testing float...\n");
	apn1 = tt_apn_alloc(0);
	apn2 = tt_apn_alloc(0);
	apn3 = tt_apn_alloc(0);

	for (int i = 0; i < count; i++) {
		double d1 = gen_float(307);
		double d2 = gen_float(307);

		/* XXX: only support same sign now */
		if (signbit(d1) != signbit(d2))
			d1 = -d1;

		tt_apn_from_float(apn1, d1);
		tt_apn_from_float(apn2, d2);

		tt_apn_add(apn3, apn1, apn2);
		tt_apn_add(apn1, apn2, apn1);

		if (_tt_apn_sanity(apn1) || _tt_apn_sanity(apn2) ||
				_tt_apn_sanity(apn3)) {
			tt_error("Sanity error");
			fail = true;
			break;
		}

		double vd1, vd2;
		tt_apn_to_float(apn3, &vd1);
		tt_apn_to_float(apn1, &vd2);

		double ds = d1 + d2;
		if (!deq(vd1, ds) || !deq(vd2, ds)) {
			tt_error("Float add error: %.18E + %.18E\n"
				       "Correct: %.18E\n"
				       "Error:   %.18E\n"
				       "Error:   %.18E",
				       d1, d2, ds, vd1, vd2);
			fail = true;
			break;
		}
	}
	tt_apn_free(apn1);
	tt_apn_free(apn2);
	tt_apn_free(apn3);
#endif

	tt_log_set_level(old_level);
}

int main(void)
{
	struct tt_apn *apn = tt_apn_alloc(50);
	char s[2000];
	double dv;

#if 1
	tt_apn_from_sint(apn, 1ULL << 63);
	tt_apn_to_string(apn, s, 100);
	printf("sint: %s == %lld\n", s, 1ULL << 63);

	tt_apn_from_uint(apn, 1234567890987654321);
	tt_apn_to_string(apn, s, 100);
	printf("uint: %s\n", s);

	tt_apn_from_uint(apn, 123456789);
	apn->_exp = -20;
	apn->_sign = 1;
	tt_apn_to_string(apn, s, 100);
	printf("str: %s\n", s);

	uint64_t dmax;
	dmax = (2046ULL << 52) | ((1ULL << 52) - 1);	/* Max double */
	tt_apn_from_float(apn, *(double *)&dmax);
	tt_apn_to_string(apn, s, 100);
	printf("max double: %s\n", s);
	dmax = 1;
	tt_apn_from_float(apn, *(double *)&dmax);
	tt_apn_to_string(apn, s, 100);
	printf("min double: %s\n", s);
#endif

	struct tt_apn *apn3 = tt_apn_alloc(320);
	tt_apn_from_string(apn3, "1.218789511116565398");
	struct tt_apn *apn2 = tt_apn_alloc(320);
	tt_apn_from_string(apn2, "2.139450735390025820");
	tt_apn_add(apn2, apn3, apn2);
	tt_apn_to_string(apn2, s, 1000);
	tt_apn_to_float(apn2, &dv);
	printf("add: %s\n     %.18E\n", s, dv);
	if (_tt_apn_sanity(apn2))
		tt_error("APN sanity error!");

	const char *sci[] = {
#if 0
		"+123", "-1234", "0", "0.00", "1.23E3", "1.2345e+3", "12.E+7",
		"12.0", "12.3456", "0.001234567", "-.23E-12", "1234.E-4", "-0",
		"-0.00", "0E+7", "-0E-7", "inf", "-NaN",
		"-", "+E1", ".", ".E12", "1.2E3.", "-3#5", ".E", "3E", "1.1.E1",
#endif
		"123456789012345678901234567890123456789012345678935234567890",
		"-999999999999999999999999999999999999999999999999999999999999",
	};
	for (int i = 0; i < sizeof(sci) / sizeof(sci[0]); i++) {
		printf("%s --> ", sci[i]);
		int err = tt_apn_from_string(apn, sci[i]);
		if (err < 0) {
			printf("Error: %d\n", err);
		} else {
			if (tt_apn_to_string(apn, s, 100))
				printf("Buffer too small\n");
			else
				printf("%s\n", s);
		}
	}

	srand(time(NULL));

	verify_add(100000);

	return 0;
}
