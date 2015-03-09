/* Key for data structures
 *
 * Copyright (C) 2014 Yibo Cai
 */
#pragma once

struct tt_key {
	int size;	/* Element size in bytes */
	int (*cmp)(const struct tt_key *num,
			const void *v1, const void *v2);/* Compare */
	void (*swap)(const struct tt_key *num,
			void *v1, void *v2);		/* Swap */

	void (*_set)(const struct tt_key *num,
			void *dst, const void *src);	/* Set value */
};
