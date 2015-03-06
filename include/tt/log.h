/* log.h
 *
 * Copyright (C) 2014 Yibo Cai
 */
#pragma once

#ifndef HAVE_CONFIG_H
#error "Include tt.h first!"
#endif

/* Log target */
enum {
	TT_LOG_TARGET_STDERR,
	TT_LOG_TARGET_FD,
	TT_LOG_TARGET_NULL,

	TT_LOG_TARGET_MAX,
};

/* Log level */
enum {
	TT_LOG_ERROR,
	TT_LOG_WARN,
	TT_LOG_INFO,
	TT_LOG_DEBUG,

	TT_LOG_MAX,
};

void tt_log_set_target(int target);
void tt_log_set_level(int level);

void tt_log_set_fd(int fd);

void tt_log(int level, const char *func, const char *format, ...);

#define tt_warn(...)	tt_log(TT_LOG_WARN, __func__, __VA_ARGS__)
#define tt_error(...)	tt_log(TT_LOG_ERROR, __func__, __VA_ARGS__)

#if (CONFIG_DEBUG_LEVEL == 0)
/* level = 0, all disabled */
#define tt_assert(b)	do {} while (0)
#define tt_assert_fa(b)	do {} while (0)
#define tt_debug(...)	do {} while (0)
#define tt_info(...)	do {} while (0)
#else
/* level > 0, assert(), tt_info() enabled */
#define tt_assert(b)						\
	do {							\
		if (__unlikely(!(b))) {				\
			tt_error("Assertion failed at %s:%u",	\
					__FILE__, __LINE__);	\
			abort();				\
		}						\
	} while (0)
#define tt_info(...)	tt_log(TT_LOG_INFO, __func__, __VA_ARGS__)
#if (CONFIG_DEBUG_LEVEL == 1)
/* level = 1, assert_fa(), debug() disabled */
#define tt_assert_fa(b)	do {} while (0)
#define tt_debug(...)	do {} while (0)
#else
/* level = 2, all enabled */
#define tt_assert_fa(b)	tt_assert(b)
#define tt_debug(...)	tt_log(TT_LOG_DEBUG, __func__, __VA_ARGS__)
#endif
#endif
