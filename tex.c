
#include <stdlib.h>
#include <ctype.h>

#include "utils.h"

#include "tex.h"

struct mapping {
	wchar_t name[32];
	wchar_t value;
};

#include "tex_table.h"


void tex_reset(struct tex *tex)
{
	free(tex->commit);
	free(tex->preedit);
	tex->commit = NULL;
	tex->preedit = NULL;
	tex->current_mapping = NULL;
	tex->buff[0] = L'\0';
}


struct tex *tex_new()
{
	struct tex *tex = calloc(1, sizeof(*tex));
	tex->buff = calloc(16, sizeof(tex->buff[0]));

	tex_reset(tex);

	return tex;
}


void tex_destroy(struct tex *tex)
{
	free(tex->buff);
	free(tex);
}


static bool tex_is_active(struct tex *tex)
{
	return wcslen(tex->buff) > 0;
}


static void tex_update_mapping(struct tex *tex)
{
	tex->current_mapping = NULL;
	size_t len = wcslen(tex->buff);
	if (len < 2) {
		// Only start matching after first actual character gets entered
		return;
	}
	for (size_t i = 0; i < ARRAY_SIZE(latex_mappings); i++) {
		// TODO: Binary search instead of this crap
		const struct mapping *m = &latex_mappings[i];
		if (wmemcmp(tex->buff, m->name, len) == 0) {
			// Depend on the order to match shortest first
			tex->current_mapping = m;
			break;
		}
	}
	if (tex->current_mapping) {
		LOGD("Updated mapping to %lc", tex->current_mapping->value);
	}
}


static void tex_clear_buff(struct tex *tex)
{
	tex->buff[0] = L'\0';
	tex_update_mapping(tex);
}


static void tex_backspace(struct tex *tex)
{
	size_t len = wcslen(tex->buff);
	if (len == 0) {
		LOGE("Attempted to backspace an empty buffer");
		return;
	}
	tex->buff[len - 1] = L'\0';
	tex_update_mapping(tex);
}


static void tex_append_to_buff(struct tex *tex, wchar_t wchr)
{
	// Current length, extra character and a terminator
	// TODO: Maybe grow only?
	size_t len = wcslen(tex->buff);
	tex->buff = reallocarray(tex->buff, len + 1 + 1, sizeof(tex->buff[0]));
	tex->buff[len++] = wchr;
	tex->buff[len] = L'\0';
	tex_update_mapping(tex);
}


static void tex_start(struct tex *tex)
{
	LOGI("Starting to match");
	tex_clear_buff(tex);
	tex_append_to_buff(tex, L'\\');
	// No selected mapping
	tex->current_mapping = NULL;
}


static void tex_reject(struct tex *tex)
{
	tex->commit = wcs_to_mbs_alloc(tex->buff);
	tex_clear_buff(tex);
	LOGI("Rejecting: '%s'", tex->commit);
}


static void tex_accept(struct tex *tex)
{
	LOGI("Accepting");
	if (tex->current_mapping != NULL) {
		wchar_t wstr[2] = {tex->current_mapping->value, L'\0'};
		tex->commit = wcs_to_mbs_alloc(wstr);
	} else {
		tex->commit = wcs_to_mbs_alloc(tex->buff);
	}
	tex_clear_buff(tex);
}


static void tex_format_preedit(struct tex *tex)
{
	// TODO: Show current selection
	wchar_t *b = wcsdup(tex->buff);
	if (tex->current_mapping != NULL) {
		// TODO: Multi-character values? Probably just force a single character...
		b[0] = tex->current_mapping->value;
	}
	tex->preedit = wcs_to_mbs_alloc(b);
	free(b);
}


bool tex_handle_key(struct tex *tex, xkb_keysym_t sym, wchar_t wchr)
{
	bool handled = false;

	if (tex->commit != NULL) {
		free(tex->commit);
		tex->commit = NULL;
	}
	if (tex->preedit != NULL) {
		free(tex->preedit);
		tex->preedit = NULL;
	}

	if (!tex_is_active(tex)) {
		if (sym == XKB_KEY_backslash) {
			tex_start(tex);
			handled = true;
		}
	} else {
		// TODO: Add an option to permanently disable in this instance
		// (for writing a lot of shit with \ and such)
		if (sym == XKB_KEY_Tab) {
			tex_accept(tex);
			handled = true;
		} else if (sym == XKB_KEY_BackSpace) {
			// TODO: Handle backspace
			tex_backspace(tex);
			handled = true;
		} else if (sym == XKB_KEY_Escape) {
			tex_reject(tex);
			// Escape is just reject, but we eat the key event
			handled = true;
		} else if (!isascii(wchr) || !isgraph(wchr) || isspace(wchr) ||
				   sym == XKB_KEY_backslash) {
			// Automatically reject on weird characters
			// Note that we do _not_ automatically start a new match on backslash
			// This is intentional - some apps get confused if both a
			// preedit string and a commit string is present.
			// TODO: Maybe add stricter filtering here?
			// Some characters can never be in the mapping key
			tex_reject(tex);
			// We do _not_ handle this key and let it propagate via the
			// virtual keyboard
			handled = false;
		} else {
			tex_append_to_buff(tex, wchr);
			handled = true;
		}
	}

	if (tex_is_active(tex)) {
		tex_format_preedit(tex);
	}
	return handled;
}
