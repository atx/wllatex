
#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>

enum log_level {
	LOG_LEVEL_DEBUG = 1,
	LOG_LEVEL_INFO = 2,
	LOG_LEVEL_WARN = 3,
	LOG_LEVEL_ERROR = 4,
};

void log_print(enum log_level level, const char *func, const char *format, ...);

#define LOGD(format, ...) log_print(LOG_LEVEL_DEBUG, __func__, format, ##__VA_ARGS__)
#define LOGI(format, ...) log_print(LOG_LEVEL_INFO, __func__, format, ##__VA_ARGS__)
#define LOGW(format, ...) log_print(LOG_LEVEL_WARN, __func__, format, ##__VA_ARGS__)
#define LOGE(format, ...) log_print(LOG_LEVEL_ERROR, __func__, format, ##__VA_ARGS__)


#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))


static inline char *wcs_to_mbs_alloc(const wchar_t *wstr)
{
	size_t res_len = wcslen(wstr)*4 + 1;
	char *str = calloc(1, res_len);
	// TODO: Assert on return value here
	wcstombs(str, wstr, res_len);
	return str;
}
