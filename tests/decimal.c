/* Test arbitrary precision decimal
 *
 * Copyright (C) 2015 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/apn/decimal.h>
#include <apn/decimal.h>

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
	if (isnan(d1))
		return isnan(d2);
	if (isinf(d1))
		return isinf(d2) && signbit(d1) == signbit(d2);
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
	struct tt_dec *dec1, *dec2, *dec3;

	int old_level = tt_log_set_level(TT_LOG_WARN);

	/* Special case */
	dec1 = tt_dec_alloc(0);
	dec2 = tt_dec_alloc(0);
	dec3 = tt_dec_alloc(0);
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
		tt_dec_from_string(dec1, cases[i].add1);
		tt_dec_from_string(dec2, cases[i].add2);
		tt_dec_add(dec3, dec1, dec2);
		tt_dec_to_string(dec3, s, 360);
		if (_tt_dec_sanity(dec3)) {
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
	tt_dec_free(dec1);
	tt_dec_free(dec2);
	tt_dec_free(dec3);
	if (fail)
		return;

	/* Big number */
	printf("Add big number...\n");
	dec1 = tt_dec_alloc(30);
	dec2 = tt_dec_alloc(50);
	dec3 = tt_dec_alloc(40);
	for (int i = 0; i < count; i++) {
		double d1, d2;
		long double ld;
		char s1[128], s2[128];
		ld = get_num(s1, 1, 100);
		tt_dec_from_string(dec1, s1);
		ld += get_num(s2, 1, 100);
		tt_dec_from_string(dec2, s2);
		double prec = 1E-15;
		if (dec1->_sign != dec2->_sign)
			prec = 1E-14;
		tt_dec_add(dec3, dec2, dec1);
		tt_dec_add(dec2, dec1, dec2);
		tt_dec_to_float(dec3, &d1);
		tt_dec_to_float(dec2, &d2);

		if (_tt_dec_sanity(dec1) || _tt_dec_sanity(dec2) ||
				_tt_dec_sanity(dec3)) {
			tt_error("Sanity error");
			fail = true;
			break;
		}

		if (!deq(ld, d1, prec) || !deq(ld, d2, prec)) {
			tt_warn("Num add mismatch: %s + %s\nMach: %.18LE\n"
					"DEC:  %.18E\nDEC:  %.18E",
					s1, s2, ld, d1, d2);
		}
	}

	tt_dec_free(dec1);
	tt_dec_free(dec2);
	tt_dec_free(dec3);
	if (fail)
		return;

#ifdef __STDC_IEC_559__
	/* Random float */
	printf("Add float...\n");
	dec1 = tt_dec_alloc(0);
	dec2 = tt_dec_alloc(0);
	dec3 = tt_dec_alloc(0);

	for (int i = 0; i < count; i++) {
		double d1 = gen_float(307);
		double d2 = gen_float(307);
		double prec = 1E-15;
		if (signbit(d1) != signbit(d2))
			prec = 1E-14;

		tt_dec_from_float(dec1, d1);
		tt_dec_from_float(dec2, d2);

		tt_dec_add(dec3, dec1, dec2);
		tt_dec_add(dec1, dec2, dec1);

		if (_tt_dec_sanity(dec1) || _tt_dec_sanity(dec2) ||
				_tt_dec_sanity(dec3)) {
			tt_error("Sanity error");
			fail = true;
			break;
		}

		double vd1, vd2;
		tt_dec_to_float(dec3, &vd1);
		tt_dec_to_float(dec1, &vd2);

		double ds = d1 + d2;
		if (!deq(vd1, ds, prec) || !deq(vd2, ds, prec)) {
			tt_warn("Float add mismatch: %.18E + %.18E\n"
					"Mach: %.18E\nDEC:  %.18E\nDEC:  %.18E",
					d1, d2, ds, vd1, vd2);
		}
	}
	tt_dec_free(dec1);
	tt_dec_free(dec2);
	tt_dec_free(dec3);
#endif

	tt_log_set_level(old_level);
}

static void verify_sub(int count)
{
	bool fail = false;
	struct tt_dec *dec1, *dec2, *dec3;

	int old_level = tt_log_set_level(TT_LOG_WARN);

	/* Big number */
	printf("Subtract big number...\n");
	dec1 = tt_dec_alloc(30);
	dec2 = tt_dec_alloc(50);
	dec3 = tt_dec_alloc(40);
	for (int i = 0; i < count; i++) {
		double d1, d2;
		long double ld;
		char s1[128], s2[128];
		ld = get_num(s1, 1, 100);
		tt_dec_from_string(dec1, s1);
		ld -= get_num(s2, 1, 100);
		tt_dec_from_string(dec2, s2);
		double prec = 1E-15;
		if (dec1->_sign == dec2->_sign)
			prec = 1E-14;
		tt_dec_sub(dec3, dec2, dec1);
		tt_dec_sub(dec2, dec1, dec2);
		tt_dec_to_float(dec3, &d1);
		tt_dec_to_float(dec2, &d2);

		if (_tt_dec_sanity(dec1) || _tt_dec_sanity(dec2) ||
				_tt_dec_sanity(dec3)) {
			tt_error("Sanity error");
			fail = true;
			break;
		}

		if (!deq(ld, -d1, prec) || !deq(ld, d2, prec)) {
			tt_warn("Num sub mismatch: %s - %s\nMach: %.18LE\n"
					"DEC:  %.18E\nDEC:  %.18E",
					s1, s2, ld, -d1, d2);
		}
	}

	tt_dec_free(dec1);
	tt_dec_free(dec2);
	tt_dec_free(dec3);
	if (fail)
		return;

#ifdef __STDC_IEC_559__
	/* Random float */
	printf("Subtract float...\n");
	dec1 = tt_dec_alloc(0);
	dec2 = tt_dec_alloc(0);
	dec3 = tt_dec_alloc(0);

	for (int i = 0; i < count; i++) {
		double d1 = gen_float(307);
		double d2 = gen_float(307);
		double prec = 1E-15;
		if (signbit(d1) == signbit(d2))
			prec = 1E-14;

		tt_dec_from_float(dec1, d1);
		tt_dec_from_float(dec2, d2);

		tt_dec_sub(dec3, dec1, dec2);
		tt_dec_sub(dec1, dec2, dec1);

		if (_tt_dec_sanity(dec1) || _tt_dec_sanity(dec2) ||
				_tt_dec_sanity(dec3)) {
			tt_error("Sanity error");
			fail = true;
			break;
		}

		double vd1, vd2;
		tt_dec_to_float(dec3, &vd1);
		tt_dec_to_float(dec1, &vd2);

		double ds = d1 - d2;
		if (!deq(vd1, ds, prec) || !deq(-vd2, ds, prec)) {
			tt_warn("Float sub mismatch: %.18E - %.18E\n"
					"Mach: %.18E\nDEC:  %.18E\nDEC:  %.18E",
					d1, d2, ds, vd1, -vd2);
		}
	}
	tt_dec_free(dec1);
	tt_dec_free(dec2);
	tt_dec_free(dec3);
#endif

	tt_log_set_level(old_level);
}

static void verify_mul(int count)
{
	bool fail = false;
	char s[1024];
	struct tt_dec *dec1, *dec2, *dec3;

	int old_level = tt_log_set_level(TT_LOG_WARN);

	/* Special case */
	dec1 = tt_dec_alloc(0);
	dec2 = tt_dec_alloc(0);
	dec3 = tt_dec_alloc(0);
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
		tt_dec_from_string(dec1, cases[i].add1);
		tt_dec_from_string(dec2, cases[i].add2);
		tt_dec_mul(dec3, dec1, dec2);
		tt_dec_to_string(dec3, s, 360);
		if (_tt_dec_sanity(dec3)) {
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
	tt_dec_free(dec1);
	tt_dec_free(dec2);
	tt_dec_free(dec3);
	if (fail)
		return;

	/* Big number */
	printf("Mul big number...\n");
	dec1 = tt_dec_alloc(30);
	dec2 = tt_dec_alloc(50);
	dec3 = tt_dec_alloc(40);
	for (int i = 0; i < count; i++) {
		double d1, d2;
		long double ld;
		char s1[128], s2[128];
		ld = get_num(s1, 1, 100);
		tt_dec_from_string(dec1, s1);
		ld *= get_num(s2, 1, 100);
		tt_dec_from_string(dec2, s2);
		double prec = 1E-15;
		tt_dec_mul(dec3, dec2, dec1);
		tt_dec_mul(dec2, dec1, dec2);
		tt_dec_to_float(dec3, &d1);
		tt_dec_to_float(dec2, &d2);

		if (_tt_dec_sanity(dec1) || _tt_dec_sanity(dec2) ||
				_tt_dec_sanity(dec3)) {
			tt_error("Sanity error");
			fail = true;
			break;
		}

		if (!deq(ld, d1, prec) || !deq(ld, d2, prec)) {
			tt_warn("Num mul mismatch: %s * %s\nMach: %.18LE\n"
					"DEC:  %.18E\nDEC:  %.18E",
					s1, s2, ld, d1, d2);
		}
	}

	tt_dec_free(dec1);
	tt_dec_free(dec2);
	tt_dec_free(dec3);
	if (fail)
		return;

#ifdef __STDC_IEC_559__
	/* Random float */
	printf("Mul float...\n");
	dec1 = tt_dec_alloc(0);
	dec2 = tt_dec_alloc(0);
	dec3 = tt_dec_alloc(0);

	for (int i = 0; i < count; i++) {
		double d1 = gen_float(150);
		double d2 = gen_float(150);
		double prec = 1E-15;

		tt_dec_from_float(dec1, d1);
		tt_dec_from_float(dec2, d2);

		tt_dec_mul(dec3, dec1, dec2);
		tt_dec_mul(dec1, dec2, dec1);

		if (_tt_dec_sanity(dec1) || _tt_dec_sanity(dec2) ||
				_tt_dec_sanity(dec3)) {
			tt_error("Sanity error");
			fail = true;
			break;
		}

		double vd1, vd2;
		tt_dec_to_float(dec3, &vd1);
		tt_dec_to_float(dec1, &vd2);

		double ds = d1 * d2;
		if (!deq(vd1, ds, prec) || !deq(vd2, ds, prec)) {
			tt_warn("Float mul mismatch: %.18E * %.18E\n"
					"Mach: %.18E\nDEC:  %.18E\nDEC:  %.18E",
					d1, d2, ds, vd1, vd2);
		}
	}
	tt_dec_free(dec1);
	tt_dec_free(dec2);
	tt_dec_free(dec3);
#endif
	tt_log_set_level(old_level);
}

static void verify_div(int count)
{
	bool fail = false;
	char s[1024];
	struct tt_dec *dec1, *dec2, *dec3;

	int old_level = tt_log_set_level(TT_LOG_WARN);

	/* Special case */
	dec1 = tt_dec_alloc(0);
	dec2 = tt_dec_alloc(0);
	dec3 = tt_dec_alloc(0);
	static const struct {
		const char *add1, *add2, *result;
	} cases[] = {
		{ "0", "0", "NaN", },
		{ "0.0", "1E-2000", "0" },
	};
	for (int i = 0; i < sizeof(cases)/sizeof(cases[0]); i++) {
		tt_dec_from_string(dec1, cases[i].add1);
		tt_dec_from_string(dec2, cases[i].add2);
		tt_dec_div(dec3, dec1, dec2);
		tt_dec_to_string(dec3, s, 360);
		if (_tt_dec_sanity(dec3)) {
			tt_error("Sanity error");
			fail = true;
			break;
		}
		if (strcmp(s, cases[i].result)) {
			tt_error("%s / %s => %s",
					cases[i].add1, cases[i].add2, s);
			fail = true;
			break;
		}
	}
	tt_dec_free(dec1);
	tt_dec_free(dec2);
	tt_dec_free(dec3);
	if (fail)
		return;

	/* Big number */
	printf("Div big number...\n");
	dec1 = tt_dec_alloc(30);
	dec2 = tt_dec_alloc(50);
	dec3 = tt_dec_alloc(40);
	for (int i = 0; i < count; i++) {
		double d1, d2;
		long double ld;
		char s1[128], s2[128];
		ld = get_num(s1, 1, 100);
		tt_dec_from_string(dec1, s1);
		ld /= get_num(s2, 1, 100);
		tt_dec_from_string(dec2, s2);
		double prec = 1E-15;
		tt_dec_div(dec3, dec1, dec2);
		tt_dec_div(dec2, dec1, dec2);
		tt_dec_to_float(dec3, &d1);
		tt_dec_to_float(dec2, &d2);

		if (_tt_dec_sanity(dec1) || _tt_dec_sanity(dec2) ||
				_tt_dec_sanity(dec3)) {
			tt_error("Sanity error");
			fail = true;
			break;
		}

		if (!deq(ld, d1, prec) || !deq(ld, d2, prec)) {
			tt_warn("Num div mismatch: %s / %s\nMach: %.18LE\n"
					"DEC:  %.18E\nDEC:  %.18E",
					s1, s2, ld, d1, d2);
		}
	}

	tt_dec_free(dec1);
	tt_dec_free(dec2);
	tt_dec_free(dec3);
	if (fail)
		return;

#ifdef __STDC_IEC_559__
	/* Random float */
	printf("Div float...\n");
	dec1 = tt_dec_alloc(0);
	dec2 = tt_dec_alloc(0);
	dec3 = tt_dec_alloc(0);

	for (int i = 0; i < count; i++) {
		double d1 = gen_float(150);
		double d2 = gen_float(150);
		double prec = 1E-15;

		tt_dec_from_float(dec1, d1);
		tt_dec_from_float(dec2, d2);

		tt_dec_div(dec3, dec1, dec2);
		tt_dec_div(dec1, dec1, dec2);

		if (_tt_dec_sanity(dec1) || _tt_dec_sanity(dec2) ||
				_tt_dec_sanity(dec3)) {
			tt_error("Sanity error");
			fail = true;
			break;
		}

		double vd1, vd2;
		tt_dec_to_float(dec3, &vd1);
		tt_dec_to_float(dec1, &vd2);

		double ds = d1 / d2;
		if (!deq(vd1, ds, prec) || !deq(vd2, ds, prec)) {
			tt_warn("Float div mismatch: %.18E * %.18E\n"
					"Mach: %.18E\nDEC:  %.18E\nDEC:  %.18E",
					d1, d2, ds, vd1, vd2);
		}
	}
	tt_dec_free(dec1);
	tt_dec_free(dec2);
	tt_dec_free(dec3);
#endif
	tt_log_set_level(old_level);
}

/* Factorial with divide and conque approach */
struct tt_dec *factorial(int i, int j)
{
	struct tt_dec *dec;

	if (i == j) {
		dec = tt_dec_alloc(0);
		tt_dec_from_uint(dec, i);
		return dec;
	}

	int m = (i + j) / 2;
	struct tt_dec *dec1 = factorial(i, m);
	struct tt_dec *dec2 = factorial(m+1, j);

	dec = tt_dec_alloc(dec1->_prec + dec2->_prec);
	tt_dec_mul(dec, dec1, dec2);

	tt_dec_free(dec1);
	tt_dec_free(dec2);

	return dec;
}

int main(void)
{
#if 0
	struct tt_dec *dec = tt_dec_alloc(50);
	char s[3000];

	tt_dec_from_sint(dec, 1ULL << 63);
	tt_dec_to_string(dec, s, 100);
	printf("sint: %s == %lld\n", s, 1ULL << 63);

	tt_dec_from_uint(dec, 1234567890987654321);
	tt_dec_to_string(dec, s, 100);
	printf("uint: %s\n", s);

	uint64_t dmax;
	dmax = (2046ULL << 52) | ((1ULL << 52) - 1);	/* Max double */
	tt_dec_from_float(dec, *(double *)&dmax);
	tt_dec_to_string(dec, s, 100);
	printf("max double: %s\n", s);
	dmax = 1;
	tt_dec_from_float(dec, *(double *)&dmax);
	tt_dec_to_string(dec, s, 100);
	printf("min double: %s\n", s);

	double dv;
	struct tt_dec *dec3 = tt_dec_alloc(320);
	tt_dec_from_string(dec3, "1.218789511116565398");
	struct tt_dec *dec2 = tt_dec_alloc(320);
	tt_dec_from_string(dec2, "2.139450735390025820");
	tt_dec_add(dec2, dec3, dec2);
	tt_dec_to_string(dec2, s, 1000);
	tt_dec_to_float(dec2, &dv);
	printf("add: %s\n     %.18E\n", s, dv);
	if (_tt_dec_sanity(dec2))
		tt_error("DEC sanity error!");
	tt_dec_free(dec2);
	tt_dec_free(dec3);

	dec3 = tt_dec_alloc(320);
	tt_dec_from_string(dec3, "541295929775964129640116467012286");
	dec2 = tt_dec_alloc(320);
	tt_dec_from_string(dec2, "541295929775964129640116467012286");
	tt_dec_sub(dec2, dec3, dec2);
	tt_dec_to_string(dec2, s, 1000);
	tt_dec_to_float(dec2, &dv);
	printf("sub: %s\n     %.18E\n", s, dv);
	if (_tt_dec_sanity(dec2))
		tt_error("DEC sanity error!");
	tt_dec_free(dec2);
	tt_dec_free(dec3);

	dec3 = tt_dec_alloc(1000);
	#define M	"99999999999999999999999999999999999999999999999999"
	#define M1000	M M M M M M M M M M M M M M M M M M M M
	tt_dec_from_string(dec3, M1000);
	dec2 = tt_dec_alloc(2000);
	tt_dec_from_string(dec2, M1000);
	tt_dec_mul(dec2, dec2, dec3);
	tt_dec_to_string(dec2, s, 3000);
	printf("mul:\n%s\n", s);
	if (_tt_dec_sanity(dec2))
		tt_error("DEC sanity error!");
	tt_dec_free(dec2);
	tt_dec_free(dec3);

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
		int err = tt_dec_from_string(dec, sci[i]);
		if (err < 0) {
			printf("Error: %d\n", err);
		} else {
			if (tt_dec_to_string(dec, s, 100))
				printf("Buffer too small\n");
			else
				printf("%s\n", s);
		}
	}
#endif

#if 0
	int old_level = tt_log_set_level(TT_LOG_WARN);
	struct tt_dec *dec_fac = factorial(1, 100000);
	char s_fac[500000];
	tt_dec_to_string(dec_fac, s_fac, 500000);
	tt_dec_free(dec_fac);
	printf("100000! = %s\n", s_fac);
	tt_log_set_level(old_level);
	return 0;
#endif

#if 0
	struct tt_dec *dividend = tt_dec_alloc(50);
	struct tt_dec *divisor = tt_dec_alloc(50);
	struct tt_dec *quotient = tt_dec_alloc(50);
	char s[100];

	tt_dec_from_uint(dividend, 10);
	tt_dec_from_uint(divisor, 8888888888);
	tt_dec_div(quotient, dividend, divisor);
	tt_dec_div(dividend, dividend, divisor);

	tt_dec_to_string(quotient, s, 100);
	printf("div: %s\n", s);
	tt_dec_to_string(dividend, s, 100);
	printf("div: %s\n", s);

	tt_dec_free(dividend);
	tt_dec_free(divisor);
	tt_dec_free(quotient);
#endif

	srand(time(NULL));

	verify_add(100000);
	verify_sub(100000);
	verify_mul(100000);
	verify_div(100000);

	return 0;
}
