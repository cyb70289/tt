/* Helpers of integer operations
 *
 * Copyright (C) 2015 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/apn/integer.h>
#include <common/lib.h>
#include "integer.h"

#include <string.h>

struct tt_int *tt_int_alloc(void)
{
	struct tt_int *ti = malloc(sizeof(struct tt_int));
	ti->sign = 0;
	ti->_max = TT_INT_DEF;
	ti->msb = 1;
	ti->buf = calloc(ti->_max, sizeof(_tt_word));
	if (ti->buf == NULL) {
		tt_error("Out of memory");
		free(ti);
		return NULL;
	}
	return ti;
}

void tt_int_free(struct tt_int *ti)
{
	free(ti->buf);
	free(ti);
}

int _tt_int_realloc(struct tt_int *ti, int msb)
{
	if (msb <= ti->_max)
		return 0;

	/* Double the buffer */
	if (msb < ti->_max * 2)
		msb = ti->_max * 2;
	_tt_word *buf = realloc(ti->buf, msb * _tt_word_sz);
	if (!buf) {
		tt_error("Out of memory");
		return TT_ENOMEM;
	}
	memset(buf + ti->_max, 0, (msb - ti->_max) * _tt_word_sz);
	ti->buf = buf;
	ti->_max = msb;
	return 0;
}

int _tt_int_copy(struct tt_int *dst, const struct tt_int *src)
{
	_tt_int_zero(dst);

	int ret = _tt_int_realloc(dst, src->msb);
	if (ret)
		return ret;

	dst->sign = src->sign;
	dst->msb = src->msb;
	memcpy(dst->buf, src->buf, src->msb * _tt_word_sz);
	return 0;
}

void _tt_int_zero(struct tt_int *ti)
{
	ti->sign = 0;
	ti->msb = 1;
	memset(ti->buf, 0, ti->_max * _tt_word_sz);
}

/* Count significant bits of a word(top bit ignored) */
int _tt_int_word_bits(_tt_word w)
{
	w <<= 1;
	for (int i = _tt_word_bits; i > 0; i--) {
		if (w & _tt_word_top_bit)
			return i;
		w <<= 1;
	}

	tt_assert(1);
	return 0;
}

/* Count trailing zeros of a word(top bit ignored) */
int _tt_int_word_ctz(_tt_word w)
{
	for (int i = 0; i < _tt_word_bits; i++) {
		if (w & 0x1)
			return i;
		w >>= 1;
	}

	tt_assert(1);
	return _tt_word_bits;
}

/* Check sanity */
int _tt_int_sanity(const struct tt_int *ti)
{
	if (ti->msb == 0 || ti->msb > ti->_max)
		return TT_APN_ESANITY;

	/* Check significands */
	if (ti->msb > 1 && ti->buf[ti->msb-1] == 0)
		return TT_APN_ESANITY;

	/* Check carry guard bit */
	for (int i = 0; i < ti->msb; i++)
		if ((ti->buf[i] & _tt_word_top_bit))
			return TT_APN_ESANITY;

	/* Check unused high words */
	for (int i = ti->msb; i < ti->_max; i++)
		if (ti->buf[i])
			return TT_APN_ESANITY;

	return 0;
}

void _tt_int_print(const struct tt_int *ti)
{
	char *s = NULL;
	tt_int_to_string(ti, &s, 10);
	printf("%s\n", s);
	free(s);
}

void _tt_int_print_buf(const _tt_word *ui, int msb)
{
	struct tt_int ti = _TT_INT_DECL(msb, ui);
	_tt_int_print(&ti);
}

/* Shift integer
 * - shift: + left, - right
 * - buf must be large enough to hold left shifted result
 * - return new msb
 * - fill empty space with zero
 */
int _tt_int_shift_buf(_tt_word *buf, int msb, int shift)
{
	int sh_words, sh_bits;
	_tt_word tmp = 0, tmp2;

	if (shift > 0) {
		sh_words = shift / _tt_word_bits;
		sh_bits = shift % _tt_word_bits;

		if (sh_words) {
			memmove(buf+sh_words, buf, msb*_tt_word_sz);
			memset(buf, 0, sh_words*_tt_word_sz);
			msb += sh_words;
		}

		if (sh_bits) {
			for (int i = sh_words; i < msb; i++) {
				tmp2 = buf[i];
				buf[i] = ((buf[i] << sh_bits) | tmp) &
					~_tt_word_top_bit;
				tmp = tmp2 >> (_tt_word_bits-sh_bits);
			}
			if (tmp)
				buf[msb++] = tmp;
		}
	} else if (shift < 0) {
		shift = -shift;
		sh_words = shift / _tt_word_bits;
		sh_bits = shift % _tt_word_bits;

		if (sh_words >= msb) {
			memset(buf, 0, msb*_tt_word_sz);
			return 1;
		}

		if (sh_words) {
			memmove(buf, buf+sh_words, (msb-sh_words)*_tt_word_sz);
			memset(buf+msb-sh_words, 0, sh_words*_tt_word_sz);
			msb -= sh_words;
		}

		if (sh_bits) {
			for (int i = msb-1; i >= 0; i--) {
				tmp2 = buf[i];
				buf[i] = (buf[i] >> sh_bits) | tmp;
				tmp = (tmp2 << (_tt_word_bits-sh_bits)) &
					~_tt_word_top_bit;
			}
			if (buf[msb-1] == 0 && msb > 1)
				msb--;
		}
	}

	return msb;
}

/* Shift integer: shift: + left, - right */
int tt_int_shift(struct tt_int *ti, int shift)
{
	if (shift > 0) {
		int ret = _tt_int_realloc(ti, ti->msb +
				(shift+_tt_word_bits-1)/_tt_word_bits);
		if (ret)
			return ret;
	}

	ti->msb = _tt_int_shift_buf(ti->buf, ti->msb, shift);
	return 0;
}
