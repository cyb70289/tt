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

bool verify(int *data)
{
	for (int i = 0; i < LEN - 1; i++)
		if (data[i] > data[i + 1])
			return false;
	return true;
}

int main(void)
{
	int *data = malloc(sizeof(int) * LEN);
	int *data_save = malloc(sizeof(int) * LEN);

	printf("Generating %d integers...\n", LEN);
	struct timeval tv;
	for (int i = 0; i < LEN; i++) {
		gettimeofday(&tv, NULL);
		srand(tv.tv_usec);
		data[i] = rand();
		data[i] /= (RAND_MAX / LEN);
		data[i] -= (LEN / 2);
	}
	memcpy(data_save, data, sizeof(int) * LEN);
	printf("Done\n\n");

	struct tt_sort_input input = {
		.num	= {
			.size	= sizeof(int),
			.type	= TT_NUM_SIGNED,
			.cmp	= NULL,
			.swap	= NULL,
		},
		.data	= data,
		.count	= LEN,
		.alg	= TT_SORT_MERGE,
	};

	memcpy(data, data_save, sizeof(int) * LEN);
	printf("Testing merge sort...\n");
	clock_t st = clock();
	tt_sort(&input);
	if (verify(data))
		printf("Done in %u ms\n", (uint)(clock() - st) / 1000);
	else
		printf("FAIL\n");
	printf("\n");

	memcpy(data, data_save, sizeof(int) * LEN);
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

	memcpy(data, data_save, sizeof(int) * LEN);
	printf("Testing quick sort(random)...\n");
	input.num.cmp = NULL;
	input.num.swap = NULL;
	input.alg = TT_SORT_QUICK;
	st = clock();
	tt_sort(&input);
	if (verify(data))
		printf("Done in %u ms\n", (uint)(clock() - st) / 1000);
	else
		printf("FAIL\n");
	printf("\n");

	printf("Testing quick sort(sorted)...\n");
	input.num.cmp = NULL;
	input.num.swap = NULL;
	input.alg = TT_SORT_QUICK;
	st = clock();
	tt_sort(&input);
	if (verify(data))
		printf("Done in %u ms\n", (uint)(clock() - st) / 1000);
	else
		printf("FAIL\n");
	printf("\n");

	printf("Testing quick sort(two value)...\n");
	for (int i = 0; i < LEN; i++)
		data[i] = (data[i] >= 0 ? 1 : -1);
	input.num.cmp = NULL;
	input.num.swap = NULL;
	input.alg = TT_SORT_QUICK;
	st = clock();
	tt_sort(&input);
	if (verify(data))
		printf("Done in %u ms\n", (uint)(clock() - st) / 1000);
	else
		printf("FAIL\n");
	printf("\n");

	printf("Testing quick sort(equal)...\n");
	for (int i = 0; i < LEN; i++)
		data[i] = 1;
	input.num.cmp = NULL;
	input.num.swap = NULL;
	input.alg = TT_SORT_QUICK;
	st = clock();
	tt_sort(&input);
	if (verify(data))
		printf("Done in %u ms\n", (uint)(clock() - st) / 1000);
	else
		printf("FAIL\n");
	printf("\n");

	memcpy(data, data_save, sizeof(int) * LEN);
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
