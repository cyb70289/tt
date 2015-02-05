/* DFT tests
 *
 * Copyright (C) 2014 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/num/dsp.h>

#include <math.h>

static void sample_sine(tt_float freq, tt_float rate, int N, tt_float (*s)[2])
{
	tt_float phase = 0;
	tt_float delta = 2 * 3.14159265 * freq / rate;

	for (int i = 0; i < N; i++) {
		s[i][0] = sin(phase);
		s[i][1] = 0;
		phase += delta;
	}
}

static void sample_rect(int ones, int N, tt_float(*s)[2])
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
	tt_float in[8][2] = {
		{ 0.3535, 0 },
		{ 0.3535, 0 },
		{ 0.6464, 0 },
		{ 1.0607, 0 },
		{ 0.3535, 0 },
		{ -1.0607, 0 },
		{ -1.3535, 0 },
		{ -0.3535, 0 },
	};
	tt_float out[8][2];

	tt_dft(in, out, 8);

	printf("DFT:\n");
	for (int i = 0; i < 8; i++)
		printf("%.4f + %.4fj\n", out[i][0], out[i][1]);

	tt_idft(out, in, 8);

	printf("\nIDFT:\n");
	for (int i = 0; i < 8; i++)
		printf("%.4f + %.4fj\n", in[i][0], in[i][1]);

	printf("\nDFT Leakage(peak at 6.2):\n");
	tt_float sample[32][2];
	sample_sine(1000, 5000, 32, sample);	/* 32 / (5000 / 1000) = 6.2 */

	tt_float dft[32][2];
	tt_dft(sample, dft, 32);

	tt_float max = 0, mag[32];
	for (int i = 0; i < 32; i++) {
		mag[i] = sqrt(dft[i][0] * dft[i][0] + dft[i][1] * dft[i][1]);
		if (mag[i] > max)
			max = mag[i];
	}

	tt_float ratio = 77 / max;
	for (int i = 0; i < 32; i++) {
		printf("%02d", i);
		int l = (int)round(mag[i] * ratio);
		while (l--)
			putchar('-');
		putchar('\n');
	};

	printf("\nSINC(5 ones):\n");
	sample_rect(5, 32, sample);
	tt_dft(sample, dft, 32);

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


	return 0;
}
