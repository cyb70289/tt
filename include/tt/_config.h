/* Basic configuration
 *
 * Copyright (C) 2015 Yibo Cai
 */

/* 1 - double, 0 - float */
#define CONFIG_DOUBLE		1

/* 0 - no assertions
 * 1 - assertions in fast path are dropped
 * 2 - debug (may introduce considerable performance penalties)
 * +---------------------------------------------+
 * | level | assert() | assert_fa() | tt_debug() |
 * +---------------------------------------------+
 * |   0   |    -     |      -      |      -     |
 * +---------------------------------------------+
 * |   1   |    v     |      -      |      -     |
 * +---------------------------------------------+
 * |   2   |    v     |      v      |      v     |
 * +---------------------------------------------+
 * v: built-in	-: discarded
 */
#define CONFIG_DEBUG_LEVEL	2
