/* Test heap
 *
 * Copyright (C) 2014 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/heap.h>

struct pair {
	int	si;	/* list index */
	int	sv;	/* value */
};

static int kway_cmp(const struct tt_key *num, const void *v1, const void *v2)
{
	return ((const struct pair *)v1)->sv - ((const struct pair *)v2)->sv;
}

/* Merge k sorted lists in O(n*logk) time with min-heap */
static void kway_merge(void)
{
	int s1[] = { 3, 15, 27, 27, 28, 99, 1001, 5666, 10087 };
	int s2[] = { 10, 11, 98, 99, 100, 200, 344, 566, 77777 };
	int s3[] = { 1, 7, 91, 101, 444, 712, 866, 5666, 7779 };
	int s4[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 };

	int *s[4] = { s1, s2, s3, s4 };
	int l[4] = { ARRAY_SIZE(s1), ARRAY_SIZE(s2),
		ARRAY_SIZE(s3), ARRAY_SIZE(s4) };

	int r[ARRAY_SIZE(s1) + ARRAY_SIZE(s2) +
		ARRAY_SIZE(s3) + ARRAY_SIZE(s4)];

	struct pair pt[4];
	struct tt_heap heap = {
		.num	= {
			.size	= sizeof(struct pair),
			.cmp	= kway_cmp,
			.swap	= NULL,
		},
		.data	= pt,
		.cap	= ARRAY_SIZE(pt),
		.htype	= TT_HEAP_MIN,
	};

	tt_heap_init(&heap);

	struct pair p;

	int idx[4] = { 0, 0, 0, 0 };
	for (int i = 0; i < 4; i++) {
		p.si = i;
		p.sv = s[i][idx[i]];
		tt_heap_insert(&heap, &p);
	}

	for (int i = 0; i < ARRAY_SIZE(r) - 4; i++) {
		/* Get minimal */
		tt_heap_extract(&heap, &p);
		r[i] = p.sv;
		/* Continue with current list */
		int nexts = p.si;
		if (idx[nexts] == (l[nexts] - 1)) {
			/* Current list is done, pick any available list */
			for (int j = 0; j < 4; j++) {
				if (j != nexts && idx[j] != (l[j] - 1)) {
					nexts = j;
					break;
				}
			}
		}
		/* Insert to heap */
		idx[nexts]++;
		p.si = nexts;
		p.sv = s[nexts][idx[nexts]];
		tt_heap_insert(&heap, &p);
	}

	/* Last 4 data */
	for (int i = ARRAY_SIZE(r) - 4; i < ARRAY_SIZE(r); i++) {
		tt_heap_extract(&heap, &p);
		r[i] = p.sv;
	}

	/* Got it! */
	for (int i = 0; i < ARRAY_SIZE(r); i++)
		printf("%d ", r[i]);
	printf("\n\n");
}

int main(void)
{
	kway_merge();

	return 0;
}
