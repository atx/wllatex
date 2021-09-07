

#include <wayland-client.h>
#include <wchar.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <locale.h>

#include <xkbcommon/xkbcommon.h>

#include "virtual-keyboard-unstable-v1-client-protocol.h"
#include "input-method-unstable-v2-client-protocol.h"
#include "text-input-unstable-v3-client-protocol.h"

#include "utils.h"
#include "tex.h"

enum state {
	STATE_DEACTIVATED = 0,
	STATE_ACTIVATING = 1,
	STATE_WAITING_FOR_ESCAPE = 1,
	STATE_BACKSLASH = 2,
	STATE_MATCHING = 3,
};

struct wllatex {
	struct wl_display *display;
	struct wl_registry *registry;
	struct wl_seat *seat;

	struct zwp_input_method_manager_v2 *ime_manager;
	struct zwp_input_method_v2 *ime;
	struct zwp_input_method_keyboard_grab_v2 *grab;
	struct zwp_virtual_keyboard_manager_v1 *virt_manager;
	struct zwp_virtual_keyboard_v1 *virt_keyboard;
	uint32_t serial;

	struct xkb_context *xkb;
	struct xkb_keymap *xkb_keymap;
	struct xkb_state *xkb_state;

	struct tex *tex;

	// Set in the IME activate/deactivate handlers, will propagate
	// to active in the done handler
	bool pending_activate;
	bool pending_deactivate;
	// Set in the IME done handler
	bool active;

	// An array containing the currently pressed keys that we handled, so
	// that we can correctly handle the release
	xkb_keycode_t pressed[64];

	struct {
		enum zwp_text_input_v3_content_hint hint;
		enum zwp_text_input_v3_content_purpose purpose;
	} content_type;
	enum zwp_text_input_v3_change_cause change_cause;
};

void fail(const char *format, ...)
{
	va_list vas;
	fprintf(stderr, "Error: ");
	va_start(vas, format);
	vfprintf(stderr, format, vas);
	va_end(vas);
	fprintf(stderr, "\n");
}


static bool handle_key_pressed(struct wllatex *wll, xkb_keycode_t keycode)
{
	// TODO: Is there a way of getting these from libxkbcommon instead of
	// hardcoding?
	static const xkb_keysym_t ignored_syms[] = {
		XKB_KEY_NoSymbol,
		XKB_KEY_Shift_L,
		XKB_KEY_Shift_R,
		XKB_KEY_Caps_Lock,
		XKB_KEY_Control_L,
		XKB_KEY_Control_R,
		XKB_KEY_Alt_L,
		XKB_KEY_Alt_R,
		XKB_KEY_Super_L,
		XKB_KEY_Super_R,
	};
	xkb_keysym_t sym = xkb_state_key_get_one_sym(wll->xkb_state, keycode);
	wchar_t wchr = xkb_state_key_get_utf32(wll->xkb_state, keycode);
	struct tex *tex = wll->tex;

	for (size_t i = 0; i < ARRAY_SIZE(ignored_syms); i++) {
		if (ignored_syms[i] == sym) {
			return false;
		}
	}

	bool handled = tex_handle_key(tex, sym, wchr);

	// Save to the pressed list
	if (handled) {
		size_t i;
		for (i = 0; i < ARRAY_SIZE(wll->pressed); i++) {
			// TODO: Maybe fail if we run out of space in the pressed array?
			if (wll->pressed[i] == 0) {
				wll->pressed[i] = keycode;
				break;
			}
		}
		if (i == ARRAY_SIZE(wll->pressed)) {
			fail("Too many pressed keys");
		}
	}

	// TODO: Proper accessors for this
	if (tex->preedit) {
		LOGI("Setting preedit to %s", tex->preedit);
		zwp_input_method_v2_set_preedit_string(wll->ime, tex->preedit,
				strlen(tex->preedit), strlen(tex->preedit));
	} else {
		LOGI("Clearing preedit");
		zwp_input_method_v2_set_preedit_string(wll->ime, "", 0, 0);
	}
	if (tex->commit) {
		LOGI("Commiting %s", tex->commit);
		zwp_input_method_v2_commit_string(wll->ime, tex->commit);
	}
	zwp_input_method_v2_commit(wll->ime, wll->serial);

	return handled;
}


static bool handle_key_released(struct wllatex *wll, xkb_keycode_t keycode)
{
	for (size_t i = 0; i < ARRAY_SIZE(wll->pressed); i++) {
		if (wll->pressed[i] == keycode) {
			wll->pressed[i] = 0;
			return true;
		}
	}
	return false;
}


static void handle_grab_key(void *data,
		struct zwp_input_method_keyboard_grab_v2 *grab,
		uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
	struct wllatex *wll = data;
	xkb_keycode_t xkb_key = key + 8;
	bool handled = false;
	switch (state) {
	case WL_KEYBOARD_KEY_STATE_PRESSED:
		handled = handle_key_pressed(wll, xkb_key);
		break;
	case WL_KEYBOARD_KEY_STATE_RELEASED:
		handled = handle_key_released(wll, xkb_key);
		break;
	default:
		LOGE("Unknown keyboard key event %d", state);
		handled = false;
		break;
	}
	if (!handled) {
		zwp_virtual_keyboard_v1_key(wll->virt_keyboard, time, key, state);
	}
}


static void handle_grab_modifiers(void *data,
		struct zwp_input_method_keyboard_grab_v2 *grab,
		uint32_t serial, uint32_t depressed, uint32_t latched, uint32_t locked,
		uint32_t group)
{
	struct wllatex *wll = data;
	if (wll->xkb_state == NULL) {
		return;
	}
	xkb_state_update_mask(wll->xkb_state, depressed, latched, locked,
			0, 0, group);
	zwp_virtual_keyboard_v1_modifiers(wll->virt_keyboard,
			depressed, latched, locked, group);
}


static void handle_grab_keymap(void *data,
		struct zwp_input_method_keyboard_grab_v2 *grab,
		uint32_t format, int32_t fd, uint32_t size)
{
	struct wllatex *wll = data;
	zwp_virtual_keyboard_v1_keymap(wll->virt_keyboard, format, fd, size);
	// Note: For some reason, this gets called repeatedly until the first key is pressed

	if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
		close(fd);
		fail("Unknown keymap format");
		return;
	}

	char *str = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (str == MAP_FAILED) {
		fail("mmap failed");
		return;
	}

	if (wll->xkb_keymap != NULL) {
		xkb_keymap_unref(wll->xkb_keymap);
	}
	wll->xkb_keymap = xkb_keymap_new_from_string(wll->xkb, str,
			XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
	munmap(str, size);
	close(fd);

	if (wll->xkb_keymap == NULL) {
		fail("Failed to parse the keymap");
	}
	if (wll->xkb_state != NULL) {
		xkb_state_unref(wll->xkb_state);
	}
	wll->xkb_state = xkb_state_new(wll->xkb_keymap);
	if (wll->xkb_state == NULL) {
		fail("Failed to create xkb_state");
	}
}


static void handle_grab_repeat_info(void *data,
		struct zwp_input_method_keyboard_grab_v2 *grab,
		int32_t rate, int32_t delay)
{
}


static const struct zwp_input_method_keyboard_grab_v2_listener grab_listener = {
	.keymap = handle_grab_keymap,
	.key = handle_grab_key,
	.modifiers = handle_grab_modifiers,
	.repeat_info = handle_grab_repeat_info
};


static void handle_ime_activate(void *data, struct zwp_input_method_v2 *ime)
{
	struct wllatex *wll = data;
	wll->pending_activate = true;
}


static void handle_ime_deactivate(void *data, struct zwp_input_method_v2 *ime)
{
	struct wllatex *wll = data;
	wll->pending_deactivate = true;
}


static void handle_ime_surrounding_text(void *data, struct zwp_input_method_v2 *ime,
		const char *text, uint32_t cursor, uint32_t anchor)
{
}

static void handle_ime_change_cause(void *data, struct zwp_input_method_v2 *ime,
		uint32_t cause)
{
}

static void handle_ime_content_type(void *data, struct zwp_input_method_v2 *ime,
		uint32_t hint, uint32_t purpose)
{
}

static void handle_ime_done(void *data, struct zwp_input_method_v2 *ime)
{
	struct wllatex *wll = data;

	if (wll->pending_activate && wll->pending_deactivate) {
		fail("Invalid state (both activate and deactivate pending)");
		return;
	}

	if (wll->pending_activate && !wll->active) {
		LOGI("Grabbing the keyboard");
		wll->grab = zwp_input_method_v2_grab_keyboard(ime);
		zwp_input_method_keyboard_grab_v2_add_listener(wll->grab,
				&grab_listener, wll);
		tex_reset(wll->tex);
		wll->active = true;
	} else if (wll->pending_deactivate && wll->active) {
		LOGI("Releasing the keyboard");
		zwp_input_method_keyboard_grab_v2_release(wll->grab);
		wll->grab = NULL;
		wll->active = false;
	}

	wll->serial++;

	wll->pending_activate = false;
	wll->pending_deactivate = false;
}

static void handle_ime_unavailable(void *data, struct zwp_input_method_v2 *ime)
{
}


static const struct zwp_input_method_v2_listener input_method_listener = {
	.activate = handle_ime_activate,
	.deactivate = handle_ime_deactivate,
	.surrounding_text = handle_ime_surrounding_text,
	.text_change_cause = handle_ime_change_cause,
	.content_type = handle_ime_content_type,
	.done = handle_ime_done,
	.unavailable = handle_ime_unavailable,
};


static void handle_registry_global(void *data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version)
{
	struct wllatex *wll = data;
	if (!strcmp(interface, wl_seat_interface.name)) {
		if (wll->seat != NULL) {
			// TODO: Add a command line switch to pick the seat, or
			// support running on multiple seats at once
			LOGE("Multiple seats are not supported!");
			return;
		}
		wll->seat = wl_registry_bind(registry, name, &wl_seat_interface, version);
	} else if (!strcmp(interface, zwp_virtual_keyboard_manager_v1_interface.name)) {
		wll->virt_manager = wl_registry_bind(registry, name,
				&zwp_virtual_keyboard_manager_v1_interface, 1);
	} else if (!strcmp(interface, zwp_input_method_manager_v2_interface.name)) {
		wll->ime_manager = wl_registry_bind(registry, name,
				&zwp_input_method_manager_v2_interface, 1);
	}
}

static void handle_registry_global_remove(void *data,
		struct wl_registry *registry, uint32_t name)
{
}


static const struct wl_registry_listener registry_listener = {
	.global = handle_registry_global,
	.global_remove = handle_registry_global_remove,
};


int main(int argc, const char *argv[])
{
	struct wllatex wll_ = { 0 };
	struct wllatex *wll = &wll_;
	// TODO: Snatch wcs to mbs from wlhangul instead of forcing a locale
	// that may not even exist on the system
	setlocale(LC_ALL, "en_US.UTF-8");

	wll->display = wl_display_connect(NULL);
	if (wll->display == NULL) {
		fail("compositor connection failed");
	}

	wll->xkb = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	wll->tex = tex_new();

	wll->registry = wl_display_get_registry(wll->display);
	wl_registry_add_listener(wll->registry, &registry_listener, wll);
	wl_display_roundtrip(wll->display);

	if (!wll->seat || !wll->ime_manager || !wll->virt_manager) {
		fail("Required protocol not supported");
	}

	wll->virt_keyboard = zwp_virtual_keyboard_manager_v1_create_virtual_keyboard(
		wll->virt_manager, wll->seat);
	wll->ime = zwp_input_method_manager_v2_get_input_method(
		wll->ime_manager, wll->seat);

	zwp_input_method_v2_add_listener(wll->ime, &input_method_listener, wll);

	while (wl_display_dispatch(wll->display) != -1) {
	}

	// TODO: Exit and destroy

}
