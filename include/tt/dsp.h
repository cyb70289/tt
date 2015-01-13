/* DSP functions
 *
 * Copyright (C) 2014 Yibo Cai
 */
#pragma once

int tt_dft(tt_float (*in)[2], tt_float (*out)[2], int N);
int tt_idft(tt_float (*in)[2], tt_float (*out)[2], int N);

int tt_fft(tt_float (*in)[2], tt_float (*out)[2], int N);
int tt_ifft(tt_float (*in)[2], tt_float (*out)[2], int N);
