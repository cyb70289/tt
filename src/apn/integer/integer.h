/* Arbitrary precision integer
 *
 * Copyright (C) 2015 Yibo Cai
 */
#pragma once

#ifdef _TT_LP64_	/* 64 bit */
typedef uint64_t _tt_word;
typedef __uint128_t _tt_word_double;
#define _tt_word_sz		8
#define _tt_word_bits		63
#define _tt_word_top_bit	(1ULL << 63)
#else			/* 32 bit */
typedef uint _tt_word;
typedef uint64_t _tt_word_double;
#define _tt_word_sz		4
#define _tt_word_bits		31
#define _tt_word_top_bit	(1U << 31)
#endif

struct tt_int {
	int sign;		/* Sign: 0/+, 1/- */

#define TT_INT_DEF	34	/* Larger than 1024/2048 bits */
	int _max;		/* Size of _int[] buffer in words */
	int msb;		/* Valid integers in buf[], 1 ~ _max */

	_tt_word *buf;		/* Each word is of 31/63 bits.
				 * Top bit reserved for carry guard.
				 */
};

#define _TT_INT_DECL(msb, i)	{ 0, msb, msb, (_tt_word *)i }

int _tt_int_realloc(struct tt_int *ti, int msb);
int _tt_int_copy(struct tt_int *dst, const struct tt_int *src);

void _tt_int_zero(struct tt_int *ti);

static inline bool _tt_int_is_zero(const struct tt_int *ti)
{
	return ti->msb == 1 && ti->buf[0] == 0;
}

int _tt_int_word_bits(_tt_word w);
int _tt_int_word_ctz(_tt_word w);
int _tt_int_sanity(const struct tt_int *ti);
void _tt_int_print(const struct tt_int *ti);
void _tt_int_print_buf(const _tt_word *ui, int msb);

int _tt_int_add_buf(_tt_word *int1, int msb1, const _tt_word *int2, int msb2);
int _tt_int_sub_buf(_tt_word *int1, int msb1, const _tt_word *int2, int msb2);
int _tt_int_mul_buf(_tt_word *intr, const _tt_word *int1, int msb1,
		const _tt_word *int2, int msb2);
int _tt_int_div_buf(_tt_word *qt, int *msb_qt, _tt_word *rm, int *msb_rm,
		const _tt_word *dd, int msb_dd, const _tt_word *ds, int msb_ds);
int _tt_int_shift_buf(_tt_word *buf, int msb, int shift);
int _tt_int_cmp_buf(const _tt_word *int1, int msb1,
		const _tt_word *int2, int msb2);
int _tt_int_get_msb(const _tt_word *ui, int len);

bool _tt_int_isprime_buf(const _tt_word *ui, int msb, int rounds);
int _tt_int_ml_rounds(int bits);	/* Miller-Rabin test rounds */
int _tt_int_mont_reduce(_tt_word *r, int *msbr, const _tt_word *c, int msbc,
		const _tt_word *u, int msbu, const _tt_word *n, int msbn,
		_tt_word *t);
uint _tt_int_mod_uint(const _tt_word *dd, int msb, uint ds);
