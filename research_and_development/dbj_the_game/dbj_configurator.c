// This is the one translation unit that owns the bodies of
// dbj_str_512_make_ok / _make_err. Every other includer of
// dbj_configurator.h gets their declarations only -- the
// exactly-once-TU convention the corelib headers are built around.
#define DBJ_MAKERESULT_IMPLEMENTATION
#include "dbj_configurator.h"

#include <stdio.h>
#include <string.h>

#define ASSETS_FOLDER "assets"

// Paths are built in place, in the storage they are returned in. Going
// through dbj_str_512_create would mean a second array of a hardcoded 512
// -- the size is already the type's, and `sizeof x.data` says so without
// repeating the number. It also skips create's memcpy, and skips its
// `const unsigned char[static 512]` parameter, which a shorter source
// cannot satisfy (-Wstringop-overread, fatal here under -Werror).
//
// Plain pointer, matching the member's type -- see the note in the header.
static dbj_str_512Result assets_path(dbj_configurator const *cfg,
                                     char const *leaf)
{
	if (!cfg->rooted) {
		return dbj_str_512_make_err(
			__func__,
			"cannot locate the running executable, so the assets "
			"folder beside it cannot be found either");
	}

	dbj_str_512 buffer = {{0}};
	// snprintf reports the length it *wanted* to write. A return at or
	// above the buffer size means the path was cut, and a cut path is a
	// wrong path -- the one outcome this whole struct exists to prevent.
	int wanted = snprintf((char *)buffer.data, sizeof buffer.data, "%s%c%s",
	                      (char const *)cfg->root.data, cfg->plat.sep, leaf);
	if (wanted < 0 || (size_t)wanted >= sizeof buffer.data) {
		char message[DBJ_RESULT_MESSAGE_SIZE] = {0};
		snprintf(message, sizeof message,
		         "asset path for '%s' does not fit in %zu bytes", leaf,
		         sizeof buffer.data);
		return dbj_str_512_make_err(__func__, message);
	}

	return dbj_str_512_make_ok(buffer);
}

dbj_configurator dbj_configurator_make(void)
{
	dbj_configurator cfg = {.assets_path = assets_path,
	                        .plat        = dbj_platform_make(),
	                        .rooted      = false};

	// The exe's own directory borrows the target's size, so the two
	// buffers cannot drift apart if the type ever changes width.
	char dir[sizeof cfg.root.data] = {0};
	if (!cfg.plat.exe_dir(dir, sizeof dir)) return cfg;

	// Assembled straight into the member it belongs to -- rooted stays
	// false unless the whole path fits, so a truncated root never becomes
	// the prefix of every asset the program asks for.
	int wanted = snprintf((char *)cfg.root.data, sizeof cfg.root.data,
	                      "%s%c%s", dir, cfg.plat.sep, ASSETS_FOLDER);
	if (wanted < 0 || (size_t)wanted >= sizeof cfg.root.data) return cfg;

	cfg.rooted = true;
	return cfg;
}
