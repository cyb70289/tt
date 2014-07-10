/* Test sort algorithms
 *
 * Copyright (C) 2014 Yibo Cai
 */
#include <tt.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sort.h>

#define LEN	(64 * 1024)

bool verify(tt_float *data)
{
	for (int i = 0; i < LEN - 1; i++)
		if (data[i] > data[i + 1])
			return false;
	return true;
}

int main(void)
{
	tt_float *data = malloc(sizeof(tt_float) * LEN);
	tt_float *data_save = malloc(sizeof(tt_float) * LEN);

	printf("Generating %d floating numbers...\n", LEN);
	struct timeval tv;
	for (int i = 0; i < LEN; i++) {
		gettimeofday(&tv, NULL);
		srand(tv.tv_usec);
		data[i] = rand();
		data[i] /= ((RAND_MAX + 1.f) / 2000.f);
		data[i] -= 1000.f;
	}
	memcpy(data_save, data, sizeof(tt_float) * LEN);
	printf("Done\n\n");

	struct tt_sort_input input = {
		.num	= {
			.size	= sizeof(tt_float),
			.type	= TT_NUM_FLOAT,
			.cmp	= NULL,
			.swap	= NULL,
		},
		.data	= data,
		.count	= LEN,
		.alg	= TT_SORT_MERGE,
	};

	memcpy(data, data_save, sizeof(tt_float) * LEN);
	printf("Testing merge sort...\n");
	clock_t st = clock();
	tt_sort(&input);
	if (verify(data))
		printf("Done in %u ms\n", (uint)(clock() - st) / 1000);
	else
		printf("FAIL\n");
	printf("\n");

	memcpy(data, data_save, sizeof(tt_float) * LEN);
	printf("Testing heap sort...\n");
	input.num.cmp = NULL;
	input.num.swap = NULL;
	input.alg = TT_SORT_HEAP;
	st = clock();
	tt_sort(&input);
	if (verify(data))
		printf("Done in %u ms\n", (uint)(clock() - st) / 1000);
	else
		printf("FAIL\n");
	printf("\n");

	memcpy(data, data_save, sizeof(tt_float) * LEN);
	printf("Testing insert sort...\n");
	input.num.cmp = NULL;
	input.num.swap = NULL;
	input.alg = TT_SORT_INSERT;
	st = clock();
	tt_sort(&input);
	if (verify(data))
		printf("Done in %u ms\n", (uint)(clock() - st) / 1000);
	else
		printf("FAIL\n");
	printf("\n");

	free(data);
	free(data_save);

	return 0;
}
