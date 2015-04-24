/* DFT tests
 *
 * Copyright (C) 2014 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/num/dft.h>

#include <math.h>
#include <time.h>
#include <string.h>

static void sample_sine(double freq, double rate, int N, double (*s)[2])
{
	double phase = 0;
	double delta = 2 * 3.14159265 * freq / rate;

	for (int i = 0; i < N; i++) {
		s[i][0] = sin(phase);
		s[i][1] = 0;
		phase += delta;
	}
}

static void sample_rect(int ones, int N, double(*s)[2])
{
	int i;
	for (i = 0; i < ones; i++) {
		s[i][0] = 1;
		s[i][1] = 0;
	}
	for (; i < N; i++)
		s[i][0]= s[i][1] = 0;
}

int main(void)
{
	double in[8][2] = {
		{ 0.3535, 0 },
		{ 0.3535, 0 },
		{ 0.6464, 0 },
		{ 1.0607, 0 },
		{ 0.3535, 0 },
		{ -1.0607, 0 },
		{ -1.3535, 0 },
		{ -0.3535, 0 },
	};
	double out[8][2];

	tt_fft(out, in, 8);

	printf("DFT:\n");
	for (int i = 0; i < 8; i++)
		printf("%.4f + %.4fj\n", out[i][0], out[i][1]);

	tt_ifft(in, out, 8);

	printf("\nIDFT:\n");
	for (int i = 0; i < 8; i++)
		printf("%.4f + %.4fj\n", in[i][0], in[i][1]);

	printf("\nDFT Leakage(peak at 6.2):\n");
	double sample[32][2];
	sample_sine(1000, 5000, 32, sample);	/* 32 / (5000 / 1000) = 6.2 */

	double dft[32][2];
	tt_fft(dft, sample, 32);

	double max = 0, mag[32];
	for (int i = 0; i < 32; i++) {
		mag[i] = sqrt(dft[i][0] * dft[i][0] + dft[i][1] * dft[i][1]);
		if (mag[i] > max)
			max = mag[i];
	}

	double ratio = 77 / max;
	for (int i = 0; i < 32; i++) {
		printf("%02d", i);
		int l = (int)round(mag[i] * ratio);
		while (l--)
			putchar('-');
		putchar('\n');
	};

	printf("\nSINC(5 ones):\n");
	sample_rect(5, 32, sample);
	tt_fft(dft, sample, 32);

	max = 0;
	for (int i = 0; i < 32; i++) {
		mag[i] = sqrt(dft[i][0] * dft[i][0] + dft[i][1] * dft[i][1]);
		if (mag[i] > max)
			max = mag[i];
	}

	ratio = 77 / max;
	for (int i = 0; i < 32; i++) {
		printf("%02d", i);
		int l = (int)round(mag[i] * ratio);
		while (l--)
			putchar('-');
		putchar('\n');
	};

	/* Compare DFT, FFT */
	double x[8192][2], X[8192][2];

	srand(time(NULL));
	for (int i = 0; i < 8192; i++) {
		x[i][0] = rand() % 1000;
		x[i][1] = rand() % 1000;

	}

	printf("8192 Points FFT...\n");
	clock_t t = clock();
	tt_fft(X, x, 8192);
	printf("Done in %u ms\n", (uint)(clock() - t) / 1000);

	printf("8192 Points DFT...\n");
	t = clock();
	tt_dft(X, x, 8192);
	printf("Done in %u ms\n", (uint)(clock() - t) / 1000);

	return 0;
}
