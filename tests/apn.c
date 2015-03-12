/* Test arbitrary precision number
 *
 * Copyright (C) 2015 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/apn/apn.h>

int main(void)
{
	struct tt_apn *apn = tt_apn_alloc(50);
	tt_apn_from_sint(apn, 1ULL << 63);
	char s[120];
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

	double dv = -123456789012345678901234567890.;
	tt_apn_from_float(apn, dv);
	tt_apn_to_string(apn, s, 100);
	printf("float: %s <--> %.18E\n", s, dv);

	struct tt_apn *apn2 = tt_apn_alloc(100);
	tt_apn_from_string(apn2, "1E-40");
	struct tt_apn *apn3 = tt_apn_alloc(100);
	tt_apn_from_string(apn3, "1E40");
	tt_apn_add(apn2, apn2, apn3);
	tt_apn_to_string(apn2, s, 120);
	printf("add: %s\n", s);

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

	return 0;
}
