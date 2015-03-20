/* Basic configuration
 *
 * Copyright (C) 2015 Yibo Cai
 */
#ifndef HAVE_TT_CONFIG_H
#define HAVE_TT_CONFIG_H

/* 0 - no assertions
 * 1 - assertions in fast path are dropped
 * 2 - debug (may introduce considerable performance penalties)
 * +-------+----------+-------------+------------+-----------+
 * | level | assert() | assert_fa() | tt_debug() | tt_info() |
 * +-------+----------+-------------+------------+-----------+
 * |   0   |    -     |      -      |      -     |     -     |
 * +-------+----------+-------------+------------+-----------+
 * |   1   |    v     |      -      |      -     |     v     |
 * +-------+----------+-------------+------------+-----------+
 * |   2   |    v     |      v      |      v     |     v     |
 * +-------+----------+-------------+------------+-----------+
 * v: built-in	-: discarded
 */
#define CONFIG_DEBUG_LEVEL	2

/* APN decimal significand length */
#define CONFIG_APN_DIGITS	60		/* 28B */
#define CONFIG_APN_DIGITS_MAX	100000000	/* 42MB */

/* Memory pool for temporary storage */
#define CONFIG_POOL_SIZE	(1 << 20)	/* Must be 2^n, >= 4096 */

#endif	/* HAVE_TT_CONFIG_H */
