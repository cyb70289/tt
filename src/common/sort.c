/* General sorting algorithms
 *
 * Copyright (C) 2014 Yibo Cai
 *
 * General programming is hard, especially for C. I struggled much and finally
 * rejected C++ template because it still couldn't meet all conditions, and I
 * really dislike C++.
 * Don't blame me on performance loss of this messy code. I think I'm correct
 * in a big picture.
 */
#include <tt.h>
#include <string.h>
#include <sort.h>
#include <heap.h>
#include <_common.h>

/* algorithm complexity statistics */
struct tt_sort_stat {
	uint	time;	/* Time cost */
	uint	space;	/* Extra space */

	/* Used only in quick sort */
	uint	depth;
	int	head;
	int	tail;
};

/* Insert sort
 * - Time: O(n^2)
 * - Space: in-place
 * - Stable
 * - Insert sort beats O(n*logn) algorithms on small array
 */
static int tt_sort_insert(struct tt_sort_input *in, struct tt_sort_stat *stat)
{
	/* Allocate buffer for one temporary element */
	char tmp[in->num.size] __align(8);
	stat->space += 1;

	void *p = in->data;
	for (int i = 1; i < in->count; i++) {
		p += in->num.size;
		/* tmp = data[i] */
		in->num._set(&in->num, tmp, p);
		/* Insert data[i] into sorted list data[0] ~ data[i-1] */
		void *q = p - in->num.size;
		for (int j = i - 1; j >= 0; q -= in->num.size, j--) {
			stat->time++;	/* Compare */
			/* if (tmp < data[j]) */
			if (in->num.cmp(&in->num, tmp, q) < 0) {
				/* data[j+1] = data[j] */
				in->num._set(&in->num, q + in->num.size, q);
				stat->time++;	/* Set */
			} else {
				break;
			}
		}
		/* data[j+1] = tmp */
		in->num._set(&in->num, q + in->num.size, tmp);
		stat->time++;	/* Set */
	}

	return 0;
}

/* Merge sort (non-recursive)
 * - Time: O(n*logn)
 * - Space: O(n)
 * - Stable
 */
static int tt_sort_merge(struct tt_sort_input *in, struct tt_sort_stat *stat)
{
	/* Allocate copy buffer */
	void *tmpbuf = malloc(in->count * in->num.size);
	if (!tmpbuf)
		return -TT_ENOMEM;
	stat->space += in->count;

	int l = 1;	/* subarray length */
	while (l < in->count) {
		/* Merge [0]~[l-1] with [l]~[2l-1],
		 *       [2l]~[3l-1] with [3l]~[4l-1], ...
		 */
		int head = 0, mid, end;
		do {
			mid = head + l - 1;
			end = tt_min(mid + l, in->count - 1);

			/* Merge [head]~[mid] with [mid+1]~[end] */
			int p = head, p1 = head, p2 = mid + 1;
			void *tmptr = tmpbuf;
			void *ptr1 = sort_ptr(in, p1);
			void *ptr2 = sort_ptr(in, p2);
			while (p <= end) {
				bool la = p2 > end;	/* Pick left array */
				if (!la) {
					if (p1 > mid) {
						la = false;
					} else {
						la = in->num.cmp(&in->num,
							ptr1, ptr2) <= 0;
						stat->time++;	/* Compare */
					}
				}

				if (la) {
					in->num._set(&in->num, tmptr, ptr1);
					p1++;
					ptr1 += in->num.size;
				} else {
					in->num._set(&in->num, tmptr, ptr2);
					p2++;
					ptr2 += in->num.size;
				}
				p++;
				tmptr += in->num.size;
				stat->time++;	/* Set */
			}
			memcpy(sort_ptr(in, head), tmpbuf,
					(end - head + 1) * in->num.size);
			stat->time += (end - head + 1);	/* Set */

			head = end + 1;
		} while (head < in->count);

		l <<= 1;
	}

	free(tmpbuf);
	return 0;
}

/* Heap sort (non-recursive)
 * - Time: O(n*logn)
 * - Space: in-place
 * - Unstable
 */
static int tt_sort_heap(struct tt_sort_input *in, struct tt_sort_stat *stat)
{
	struct tt_heap heap = {
		.num	= in->num,
		.data	= in->data,
		.cap	= in->count,
		.htype	= TT_HEAP_MAX,
	};

	int ret = tt_heap_init(&heap);
	if (ret)
		return ret;

	/* Build heap */
	tt_heap_build(&heap, heap.cap);

	void *head = heap.data;
	heap.__heaplen--;
	void *end = sort_ptr(&heap, heap.__heaplen);
	for (; heap.__heaplen >= 1; heap.__heaplen--) {
		/* NOTE: Heap length is __heaplen+1 */
		/* Swap [0] with [__heaplen] */
		heap.num.swap(&heap.num, head, end);
		end -= heap.num.size;
		/* [__heaplen] is OK, let's heapify [0] ~ [__heaplen-1] */
		tt_heap_heapify(&heap, 0);
	}

	return 0;
}

/* Quick sort
 * - Time: average - O(n*logn), worst - O(n^2)
 * - Space: O(logn) recursive stack
 * - Unstable
 * - Based on "Engineering A Sort Function" by Bently & McIlroy. This code
 *   performs stable upon random, sorted, and nearly equal lists.
 */
static int tt_sort_quick(struct tt_sort_input *in, struct tt_sort_stat *stat)
{
	stat->depth++;	/* Max stack depth */
	if (stat->depth > stat->space)
		stat->space = stat->depth;

	/* Buffer to hold pivot value */
	char pivot[in->num.size] __align(8);

	int p = stat->head;
	int r = stat->tail;
	while (p < r) {
		/* Select pivot */
		void *vp = sort_ptr(in, p);
		void *vr = sort_ptr(in, r);
		/* Pick middle element as pivot if short array */
		void *vq = sort_ptr(in, (p + r) / 2);
		void *vm = vq;
		if (r - p >= 15) {
			/* Pick medium-3 as pivot if long array */
			if (in->num.cmp(&in->num, vp, vq) < 0) {
				vm = vp;
				if (in->num.cmp(&in->num, vp, vr) < 0) {
					vm = vr;
					if (in->num.cmp(&in->num, vq, vr) < 0)
						vm = vq;
				}
			} else {
				vm = vq;
				if (in->num.cmp(&in->num, vq, vr) < 0) {
					vm = vr;
					if (in->num.cmp(&in->num, vp, vr) < 0)
						vm = vp;
				}
			}
		}
		/* Save pivot value */
		in->num._set(&in->num, pivot, vm);

		/* Split-end partition */
		void *pa = vp, *pb = vp;
		void *pc = vr, *pd = vr;
		while (1) {
			int r;
			while (pb <= pc &&
				(r = in->num.cmp(&in->num, pb, pivot)) <= 0) {
				if (r == 0) {
					in->num.swap(&in->num, pa, pb);
					pa += in->num.size;
					stat->time++;
				}
				pb += in->num.size;
				stat->time++;
			}
			while (pc >= pb &&
				(r = in->num.cmp(&in->num, pc, pivot)) >= 0) {
				if (r == 0) {
					in->num.swap(&in->num, pc, pd);
					pd -= in->num.size;
					stat->time++;
				}
				pc -= in->num.size;
				stat->time++;
			}
			if (pb > pc)
				break;
			in->num.swap(&in->num, pb, pc);
			pb += in->num.size;
			pc -= in->num.size;
			stat->time++;
		}
		/* Current layout
		 *  +-----+---------+---------+-----+
		 *  |  =  |    <    |    >    |  =  |
		 *  +-----+---------+---------+-----+
		 *  vp    pa      pc pb       pd   rp
		 */

		int l = tt_min(pa - vp, pb - pa);
		void *vt = pb - l;
		while (l) {
			in->num.swap(&in->num, vp, vt);
			vp += in->num.size;
			vt += in->num.size;
			l -= in->num.size;
			stat->time++;
		}
		l = tt_min(pd - pc, vr - pd);
		vt = pb;
		vr -= (l - in->num.size);
		while (l) {
			in->num.swap(&in->num, vt, vr);
			vt += in->num.size;
			vr += in->num.size;
			l -= in->num.size;
			stat->time++;
		}
		int q1 = p + (pb - pa) / in->num.size - 1;
		int q2 = r - (pd - pc) / in->num.size + 1;
		/* Current layout
		 *  +---------+---------+----------+
		 *  |    <    |    =    |    >     |
		 *  +---------+---------+----------+
		 *  p        q1         q2         r
		 */

		/* Call upon short list recursively, leave long one iterated */
		if (q1 - p <= r - q2) {
			stat->head = p;
			stat->tail = q1;
			tt_sort_quick(in, stat);
			p = q2;
		} else {
			stat->head = q2;
			stat->tail = r;
			tt_sort_quick(in, stat);
			r = q1;
		}
	}

	stat->depth--;
	return 0;
}

static int (*tt_sort_internal[])(struct tt_sort_input *,
		struct tt_sort_stat *stat) = {
	[TT_SORT_INSERT]	= tt_sort_insert,
	[TT_SORT_MERGE]		= tt_sort_merge,
	[TT_SORT_HEAP]		= tt_sort_heap,
	[TT_SORT_QUICK]		= tt_sort_quick,
};

int tt_sort(struct tt_sort_input *input)
{
	tt_assert(input->alg >= 0 && input->alg < TT_SORT_MAX);

	_tt_num_select(&input->num);

	struct tt_sort_stat stat;
	memset(&stat, 0, sizeof(struct tt_sort_stat));
	stat.tail = input->count - 1;	/* Only for quick sort */
	int ret = tt_sort_internal[input->alg](input, &stat);

	tt_debug("time: %u, space: %u", stat.time, stat.space);

	return ret;
}
