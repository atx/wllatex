
#pragma once

#include <wchar.h>
#include <stdbool.h>
#include <xkbcommon/xkbcommon.h>

_Static_assert(sizeof(wchar_t) == 4, "Invalid wchar_t size");

struct mapping;

struct tex {
	wchar_t *buff;

	struct mapping *current_mapping;
	char *preedit;
	char *commit;
};

struct tex *tex_new();
void tex_destroy(struct tex *tex);
void tex_reset(struct tex *tex);
bool tex_handle_key(struct tex *tex, xkb_keysym_t sym, wchar_t wchr);
const wchar_t *tex_get_preedit_string(struct tex *tex);
const wchar_t *tex_get_commit_string(struct tex *tex);
