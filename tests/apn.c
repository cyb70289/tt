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

static bool deq(double d1, double d2, double prec)
{
	if (d1 == 0)
		return d2 == 0;
	long double dd = (long double)d1 - (long double)d2;
	dd /= d1;
	return dd < prec;
}

static long double get_num(char *s, int lmin, int lmax)
{
	long double ld = 0;

	s[0] = ((rand() & 1)) ? '-' : '+';

	/* len: lmin+1 ~ lmax+1 */
	int len = lmin + rand() % (lmax - lmin + 1) + 1;
	for (int i = 1; i < len; i++) {
		s[i] = rand() % 10;
		ld *= 10;
		ld += s[i];
		s[i] += '0';
	}
	s[len] = '\0';

	if (s[0] == '-')
		ld = -ld;

	return ld;
}

static void verify_add(int count)
{
	bool fail = false;
	char s[1024];
	struct tt_apn *apn1, *apn2, *apn3;

	int old_level = tt_log_set_level(TT_LOG_WARN);

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
		{ "-0", "0", "-0", },
		{ "0E-10", "-0", "-0E-10" },
		{ "-0E-2000", "0", "0E-2000" },
		{ "0E-20", "-0E-10", "-0E-20" },
		{ "-0E-20", "0E-1000", "-0E-1000" },
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
	printf("Add big number...\n");
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
		double prec = 1E-15;
		if (apn1->_sign != apn2->_sign)
			prec = 1E-14;
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

		if (!deq(ld, d1, prec) || !deq(ld, d2, prec)) {
			tt_warn("Num add mismatch: %s + %s\nMach: %.18LE\n"
					"APN:  %.18E\nAPN:  %.18E",
					s1, s2, ld, d1, d2);
		}
	}

	tt_apn_free(apn1);
	tt_apn_free(apn2);
	tt_apn_free(apn3);
	if (fail)
		return;

#ifdef __STDC_IEC_559__
	/* Random float */
	printf("Add float...\n");
	apn1 = tt_apn_alloc(0);
	apn2 = tt_apn_alloc(0);
	apn3 = tt_apn_alloc(0);

	for (int i = 0; i < count; i++) {
		double d1 = gen_float(307);
		double d2 = gen_float(307);
		double prec = 1E-15;
		if (signbit(d1) != signbit(d2))
			prec = 1E-14;

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
		if (!deq(vd1, ds, prec) || !deq(vd2, ds, prec)) {
			tt_warn("Float add mismatch: %.18E + %.18E\n"
					"Mach: %.18E\nAPN:  %.18E\nAPN:  %.18E",
					d1, d2, ds, vd1, vd2);
		}
	}
	tt_apn_free(apn1);
	tt_apn_free(apn2);
	tt_apn_free(apn3);
#endif

	tt_log_set_level(old_level);
}

static void verify_sub(int count)
{
	bool fail = false;
	struct tt_apn *apn1, *apn2, *apn3;

	int old_level = tt_log_set_level(TT_LOG_WARN);

	/* Big number */
	printf("Subtract big number...\n");
	apn1 = tt_apn_alloc(30);
	apn2 = tt_apn_alloc(50);
	apn3 = tt_apn_alloc(40);
	for (int i = 0; i < count; i++) {
		double d1, d2;
		long double ld;
		char s1[128], s2[128];
		ld = get_num(s1, 1, 100);
		tt_apn_from_string(apn1, s1);
		ld -= get_num(s2, 1, 100);
		tt_apn_from_string(apn2, s2);
		double prec = 1E-15;
		if (apn1->_sign == apn2->_sign)
			prec = 1E-14;
		tt_apn_sub(apn3, apn2, apn1);
		tt_apn_sub(apn2, apn1, apn2);
		tt_apn_to_float(apn3, &d1);
		tt_apn_to_float(apn2, &d2);

		if (_tt_apn_sanity(apn1) || _tt_apn_sanity(apn2) ||
				_tt_apn_sanity(apn3)) {
			tt_error("Sanity error");
			fail = true;
			break;
		}

		if (!deq(ld, -d1, prec) || !deq(ld, d2, prec)) {
			tt_warn("Num sub mismatch: %s - %s\nMach: %.18LE\n"
					"APN:  %.18E\nAPN:  %.18E",
					s1, s2, ld, -d1, d2);
		}
	}

	tt_apn_free(apn1);
	tt_apn_free(apn2);
	tt_apn_free(apn3);
	if (fail)
		return;

#ifdef __STDC_IEC_559__
	/* Random float */
	printf("Subtract float...\n");
	apn1 = tt_apn_alloc(0);
	apn2 = tt_apn_alloc(0);
	apn3 = tt_apn_alloc(0);

	for (int i = 0; i < count; i++) {
		double d1 = gen_float(307);
		double d2 = gen_float(307);
		double prec = 1E-15;
		if (signbit(d1) == signbit(d2))
			prec = 1E-14;

		tt_apn_from_float(apn1, d1);
		tt_apn_from_float(apn2, d2);

		tt_apn_sub(apn3, apn1, apn2);
		tt_apn_sub(apn1, apn2, apn1);

		if (_tt_apn_sanity(apn1) || _tt_apn_sanity(apn2) ||
				_tt_apn_sanity(apn3)) {
			tt_error("Sanity error");
			fail = true;
			break;
		}

		double vd1, vd2;
		tt_apn_to_float(apn3, &vd1);
		tt_apn_to_float(apn1, &vd2);

		double ds = d1 - d2;
		if (!deq(vd1, ds, prec) || !deq(-vd2, ds, prec)) {
			tt_warn("Float sub mismatch: %.18E - %.18E\n"
					"Mach: %.18E\nAPN:  %.18E\nAPN:  %.18E",
					d1, d2, ds, vd1, -vd2);
		}
	}
	tt_apn_free(apn1);
	tt_apn_free(apn2);
	tt_apn_free(apn3);
#endif

	tt_log_set_level(old_level);
}

static void verify_mul(int count)
{
	bool fail = false;
	char s[1024];
	struct tt_apn *apn1, *apn2, *apn3;

	int old_level = tt_log_set_level(TT_LOG_WARN);

	/* Special case */
	apn1 = tt_apn_alloc(0);
	apn2 = tt_apn_alloc(0);
	apn3 = tt_apn_alloc(0);
	static const struct {
		const char *add1, *add2, *result;
	} cases[] = {
		{ "0", "0", "0", },
		{ "0E-10", "-0E-20", "-0E-30" },
		{ "1E-2000", "0.0", "0E-2001" },
		{ "0.0", "1E-2000", "0E-2001" },
		{ "100E-10", "-0.0", "-0E-9" },
		{ "100E-10", "0", "0" },

	};
	for (int i = 0; i < sizeof(cases)/sizeof(cases[0]); i++) {
		tt_apn_from_string(apn1, cases[i].add1);
		tt_apn_from_string(apn2, cases[i].add2);
		tt_apn_mul(apn3, apn1, apn2);
		tt_apn_to_string(apn3, s, 360);
		if (_tt_apn_sanity(apn3)) {
			tt_error("Sanity error");
			fail = true;
			break;
		}
		if (strcmp(s, cases[i].result)) {
			tt_error("%s * %s => %s",
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
	printf("Mul big number...\n");
	apn1 = tt_apn_alloc(30);
	apn2 = tt_apn_alloc(50);
	apn3 = tt_apn_alloc(40);
	for (int i = 0; i < count; i++) {
		double d1, d2;
		long double ld;
		char s1[128], s2[128];
		ld = get_num(s1, 1, 100);
		tt_apn_from_string(apn1, s1);
		ld *= get_num(s2, 1, 100);
		tt_apn_from_string(apn2, s2);
		double prec = 1E-15;
		tt_apn_mul(apn3, apn2, apn1);
		tt_apn_mul(apn2, apn1, apn2);
		tt_apn_to_float(apn3, &d1);
		tt_apn_to_float(apn2, &d2);

		if (_tt_apn_sanity(apn1) || _tt_apn_sanity(apn2) ||
				_tt_apn_sanity(apn3)) {
			tt_error("Sanity error");
			fail = true;
			break;
		}

		if (!deq(ld, d1, prec) || !deq(ld, d2, prec)) {
			tt_warn("Num mul mismatch: %s * %s\nMach: %.18LE\n"
					"APN:  %.18E\nAPN:  %.18E",
					s1, s2, ld, d1, d2);
		}
	}

	tt_apn_free(apn1);
	tt_apn_free(apn2);
	tt_apn_free(apn3);
	if (fail)
		return;

#ifdef __STDC_IEC_559__
	/* Random float */
	printf("Mul float...\n");
	apn1 = tt_apn_alloc(0);
	apn2 = tt_apn_alloc(0);
	apn3 = tt_apn_alloc(0);

	for (int i = 0; i < count; i++) {
		double d1 = gen_float(150);
		double d2 = gen_float(150);
		double prec = 1E-15;

		tt_apn_from_float(apn1, d1);
		tt_apn_from_float(apn2, d2);

		tt_apn_mul(apn3, apn1, apn2);
		tt_apn_mul(apn1, apn2, apn1);

		if (_tt_apn_sanity(apn1) || _tt_apn_sanity(apn2) ||
				_tt_apn_sanity(apn3)) {
			tt_error("Sanity error");
			fail = true;
			break;
		}

		double vd1, vd2;
		tt_apn_to_float(apn3, &vd1);
		tt_apn_to_float(apn1, &vd2);

		double ds = d1 * d2;
		if (!deq(vd1, ds, prec) || !deq(vd2, ds, prec)) {
			tt_warn("Float mul mismatch: %.18E * %.18E\n"
					"Mach: %.18E\nAPN:  %.18E\nAPN:  %.18E",
					d1, d2, ds, vd1, vd2);
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
#if 0
	struct tt_apn *apn = tt_apn_alloc(50);
	char s[3000];

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

	double dv;
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
	tt_apn_free(apn2);
	tt_apn_free(apn3);

	apn3 = tt_apn_alloc(320);
	tt_apn_from_string(apn3, "541295929775964129640116467012286");
	apn2 = tt_apn_alloc(320);
	tt_apn_from_string(apn2, "541295929775964129640116467012286");
	tt_apn_sub(apn2, apn3, apn2);
	tt_apn_to_string(apn2, s, 1000);
	tt_apn_to_float(apn2, &dv);
	printf("sub: %s\n     %.18E\n", s, dv);
	if (_tt_apn_sanity(apn2))
		tt_error("APN sanity error!");
	tt_apn_free(apn2);
	tt_apn_free(apn3);

	apn3 = tt_apn_alloc(1000);
	#define M	"99999999999999999999999999999999999999999999999999"
	#define M1000	M M M M M M M M M M M M M M M M M M M M
	tt_apn_from_string(apn3, M1000);
	apn2 = tt_apn_alloc(2000);
	tt_apn_from_string(apn2, M1000);
	tt_apn_mul(apn2, apn2, apn3);
	tt_apn_to_string(apn2, s, 3000);
	printf("mul:\n%s\n", s);
	if (_tt_apn_sanity(apn2))
		tt_error("APN sanity error!");
	tt_apn_free(apn2);
	tt_apn_free(apn3);

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
#endif

	srand(time(NULL));

	verify_add(100000);
	verify_sub(100000);
	verify_mul(100000);

	return 0;
}
