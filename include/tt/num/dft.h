/* DSP functions
 *
 * Copyright (C) 2014 Yibo Cai
 */
#pragma once

int tt_dft(double (*in)[2], double (*out)[2], int N);
int tt_idft(double (*in)[2], double (*out)[2], int N);

int tt_fft(double (*in)[2], double (*out)[2], int N);
int tt_ifft(double (*in)[2], double (*out)[2], int N);
