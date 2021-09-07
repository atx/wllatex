
#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>

#define LOG_BASE(level, format, ...) fprintf(stderr, "[%s][%s] " format "\n", level, __func__, ##__VA_ARGS__)
// TODO: Make this configurable
#define LOGD(...) LOG_BASE("DBG", __VA_ARGS__)
#define LOGI(...) LOG_BASE("INF", __VA_ARGS__)
#define LOGE(...) LOG_BASE("ERR", __VA_ARGS__)


#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))


static inline char *wcs_to_mbs_alloc(const wchar_t *wstr)
{
	size_t res_len = wcslen(wstr)*4 + 1;
	char *str = calloc(1, res_len);
	// TODO: Assert on return value here
	wcstombs(str, wstr, res_len);
	return str;
}
