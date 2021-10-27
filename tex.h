
#pragma once

#include <stdbool.h>
#include <xkbcommon/xkbcommon.h>

#include "utf8.h"

struct mapping;

struct tex {
	ucschar buff[64];
	size_t buff_len;

	const struct mapping *current_mapping;
	char *preedit;
	char *commit;
};

struct tex *tex_new();
void tex_destroy(struct tex *tex);
void tex_reset(struct tex *tex);
bool tex_handle_key(struct tex *tex, xkb_keysym_t sym, ucschar wchr);
const ucschar *tex_get_preedit_string(struct tex *tex);
const ucschar *tex_get_commit_string(struct tex *tex);
