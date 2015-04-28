/* DFT tests
 *
 * Copyright (C) 2014 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/num/dft.h>
#include <tt/num/complex.h>

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

	double in2[8];
	for (int i = 0; i < 8; i++)
		in2[i] = in[i][0];
	tt_fft_real(out, in2, 8);
	printf("\nDFT(real):\n");
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

#if 0
	/* FFT fast multiplication */
	uint ai = 12345678, bi = 87654321;
	double a[4] = { ai & 0xFFFF, ai >> 16, 0 , 0 };
	double b[4] = { bi & 0xFFFF, bi >> 16, 0 , 0 };
	double c[4][2];
	double A[4][2], B[4][2], C[4][2];
	tt_fft_real(A, a, 4);
	tt_fft_real(B, b, 4);
	for (int i = 0; i < 4; i++)
		tt_complex_mul(C[i], A[i], B[i]);
	tt_ifft(c, C, 4);
	printf("%u * %u = ", ai, bi);
	for (int i = 2; i >= 0; i--) {
		printf("(%.0f << %d)", c[i][0], 16 * i);
		if (i)
			printf(" + ");
	}
	printf("\n");
#endif

#if 0
	/* FFT rounding error */
	#define N	1024
	double a[N*2], b[N*2], c[N*2][2];
	double A[N*2][2], B[N*2][2], C[N*2][2];
	for (int i = 0; i < N; i++)
		a[i] = b[i] = 65535;
	for (int i = N; i < N*2; i++)
		a[i] = b[i] = 0;
	tt_fft_real(A, a, N*2);
	tt_fft_real(B, b, N*2);
	for (int i = 0; i < N*2; i++)
		tt_complex_mul(C[i], A[i], B[i]);
	tt_ifft(c, C, N*2);
	for (int i = 0; i < N*2; i++)
		if (fabs(c[i][1]) >  0.4)
			printf("%f %f\n", c[i][0], c[i][1]);
#endif

	return 0;
}
