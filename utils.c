
#include <stdarg.h>
#include <string.h>

#include "utils.h"


static const char *level_prefixes[] = {
	[LOG_LEVEL_DEBUG] =		"\x1b[97m[DBG]",
	[LOG_LEVEL_INFO] =		"\x1b[1m\x1b[94m[INF]",
	[LOG_LEVEL_WARN] =		"\x1b[1m\x1b[93m[WRN]",
	[LOG_LEVEL_ERROR] =		"\x1b[1m\x1b[91m[ERR]",
};


static enum log_level min_log_level = LOG_LEVEL_INFO;


void log_set_level(enum log_level level)
{
	min_log_level = level;
}


void log_print(enum log_level level, const char *func, const char *format, ...)
{
	if (level < min_log_level) {
		return;
	}
	const char *prefix = "\x1b[1m\x1b[91m[XXX]";
	char fnpad[30] = { 0 };
	if (level < ARRAY_SIZE(level_prefixes) && level >= LOG_LEVEL_DEBUG) {
		prefix = level_prefixes[level];
	}
	snprintf(fnpad, sizeof(fnpad), "[%s]", func);
	fnpad[strlen(fnpad) - 1] = ']';
	for (size_t i = strlen(fnpad); i < sizeof(fnpad) - 1; i++) {
		fnpad[i] = ' ';
	}

	fputs(prefix, stderr);
	fputs(fnpad, stderr);

	va_list vas;
	va_start(vas, format);
	vfprintf(stderr, format, vas);
	va_end(vas);
	fputc('\n', stderr);
	fflush(stderr);
}


char *wcs_to_mbs_alloc(const wchar_t *wstr)
{
	size_t res_len = wcslen(wstr)*4 + 1;
	char *str = calloc(1, res_len);
	// TODO: Assert on return value here
	wcstombs(str, wstr, res_len);
	return str;
}
