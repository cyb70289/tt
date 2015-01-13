/* Test matrix
 *
 * Copyright (C) 2014 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/matrix.h>

#include <string.h>

int main(void)
{
	int r;
	struct tt_mtx m, b, b2;
	tt_float vm[6][6] = {
		{ 10, 15, 11, 6, 1, 1 },
		{ 106, 15, 114, 63, 12, 11 },
		{ 1, -82, 83, -4, 5, -86 },
		{ -16, 15, -14, 13, -12, 11 },
		{ 11, 61, 21, 401, 355, 511 },
		{ 50, 30, 40, 20, 60, 10 },
	};
	tt_float vb[6] = { 100, 99, 101, 98, -200, -321 };
	tt_float vm2[6][6];
	memcpy(vm2, vm, sizeof(vm));

	m.cols = m.rows = 6;
	m.v = (tt_float *)vm;

	b.cols = 1;
	b.rows = 6;
	b.v = vb;

	r = tt_mtx_gaussj(&m, &b);
	printf("%f, %f, %f, %f, %f, %f\n",
			vb[0], vb[1], vb[2], vb[3], vb[4], vb[5]);
	printf("%d\n", r);

	int i, j;
	for (i = 0; i < 6; i++) {
		for (j = 0; j < 6; j++)
			printf("%f, ", m.v[i*6+j]);
		printf("\n");
	}

	m.v = (tt_float *)vm2;
	b2.cols = 1;
	b2.rows = 6;
	tt_float vb2[6];
	b2.v = vb2;
	tt_mtx_mul(&m, &b, &b2);
	for (i = 0; i < 6; i++)
		printf("%f, ", b2.v[i]);
	printf("\n");

	return r;
}
