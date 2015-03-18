/* log.c
 *
 * Copyright (C) 2014 Yibo Cai
 */
#include <tt/tt.h>

#include <string.h>

static char level2char[] = {
	[TT_LOG_ERROR]  = 'E',
	[TT_LOG_WARN]   = 'W',
	[TT_LOG_INFO]   = 'I',
	[TT_LOG_DEBUG]  = 'D',
};

static int __target = TT_LOG_TARGET_STDERR;
static int __fd = -1;
static int __level = TT_LOG_DEBUG;

void tt_log_set_target(int target)
{
	tt_assert(target >= 0 && target < TT_LOG_TARGET_MAX);

	__target = target;
}

int tt_log_set_level(int level)
{
	tt_assert(level >= 0 && level < TT_LOG_MAX);

	int old_level = __level;
	__level = level;

	return old_level;
}

void tt_log_set_fd(int fd)
{
	if (fd >= 0) {
		__fd = fd;
	} else if (__fd >= 0) {
		tt_log_set_target(TT_LOG_TARGET_STDERR);
		close(__fd);
		__fd = -1;
	}
}

void tt_log(int level, const char *func, const char *format, ...)
{
	tt_assert(level >= 0 && level < TT_LOG_MAX);

	if (__target == TT_LOG_TARGET_NULL)
		return;
	if (level > __level)
		return;

	va_list ap;
	va_start(ap, format);

	char msg[512+12];
	if (vsnprintf(msg, 512, format, ap) >= 512)
		snprintf(msg+511, 13, "<Truncated>");

	char meta[32];
	const char *prefix = "", *suffix = "";

	switch (__target) {
	case TT_LOG_TARGET_STDERR:
		if (level == TT_LOG_ERROR)
			prefix = "\x1B[1;31m";	/* Red bold */
		else if (level == TT_LOG_WARN)
			prefix = "\x1B[1;33m";	/* Yellow bold */
		else if (level == TT_LOG_INFO)
			prefix = "\x1B[1;37m";  /* White bold */
		if (prefix[0])
			suffix = "\x1B[0m";
		fprintf(stderr, "[%c]%s: %s%s%s\n", level2char[level], func,
				prefix, msg, suffix);
		break;

	case TT_LOG_TARGET_FD:
		snprintf(meta, 32, "\n[%c]%s: ", level2char[level], func);
		if (write(__fd, meta, strlen(meta)) < 0 ||
				write(__fd, msg, strlen(msg)) < 0) {
			tt_log_set_fd(-1);
			fprintf(stderr, "Error writing logs to file descriptor,"
					" redirect messges to console.\n");
			tt_log_set_target(TT_LOG_TARGET_STDERR);
		}
		break;

	default:
		break;
	}

	va_end(ap);
}
