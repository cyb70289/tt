/* Mersenne-Twister pesudo random number generator
 *
 * Copyright (C) 2015 Yibo Cai
 */
#include <tt/tt.h>
#include "lib.h"

#include <time.h>
#include <pthread.h>

#define MT_CNT	624	/* State vector count */
#define MT_OFF	397	/* Recurrence offset */

static int _si = -1;
static uint _sv[MT_CNT];
static uint _sx[2] = { 0, 0x9908B0DF };

static pthread_mutex_t _mtx = PTHREAD_MUTEX_INITIALIZER;

static uint __tt_rand(void)
{
	uint r;

	if (_si == MT_CNT) {
		int i = 0;

		for (; i < MT_CNT-MT_OFF; i++) {
			r = (_sv[i] & BIT(31)) | (_sv[i+1] & ~BIT(31));
			_sv[i] = _sv[i+MT_OFF] ^ (r >> 1) ^ _sx[r & 0x1];
		}
		for (; i < MT_CNT-1; i++) {
			r = (_sv[i] & BIT(31)) | (_sv[i+1] & ~BIT(31));
			_sv[i] = _sv[i+MT_OFF-MT_CNT] ^ (r >> 1) ^ _sx[r & 0x1];
		}
		r = (_sv[MT_CNT-1] & BIT(31)) | (_sv[0] & ~BIT(31));
		_sv[MT_CNT-1] = _sv[MT_OFF-1] ^ (r >> 1) ^ _sx[r & 0x1];

		_si = 0;
	}

	r = _sv[_si++];
	r ^= r >> 11;
	r ^= (r << 7) & 0x9D2C5680;
	r ^= (r << 15) & 0xEFC60000;
	r ^= r >> 18;

	return r;
}

static void __tt_srand(uint seed)
{
	_sv[0] = seed;
	for (int i = 1; i < MT_CNT; i++)
		_sv[i] = 1812433253ULL * (_sv[i-1] ^ (_sv[i-1] >> 30)) + i;
	_si = MT_CNT;
}

void _tt_srand(uint seed)
{
	pthread_mutex_lock(&_mtx);
	__tt_srand(seed);
	pthread_mutex_unlock(&_mtx);
}

uint _tt_rand(void)
{
	uint r;

	pthread_mutex_lock(&_mtx);
	if (_si == -1)
		__tt_srand(clock());
	r = __tt_rand();
	pthread_mutex_unlock(&_mtx);

	return r;
}
