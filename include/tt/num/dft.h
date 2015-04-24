/* DSP functions
 *
 * Copyright (C) 2014 Yibo Cai
 */
#pragma once

int tt_dft(double (*out)[2], double (*in)[2], uint N);
int tt_idft(double (*out)[2], double (*in)[2], uint N);

int tt_fft(double (*out)[2], double (*in)[2], uint N);
int tt_ifft(double (*out)[2], double (*in)[2], uint N);
