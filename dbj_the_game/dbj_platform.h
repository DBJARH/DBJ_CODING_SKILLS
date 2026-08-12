#ifndef DBJ_THE_GAME_DBJ_PLATFORM_H
#define DBJ_THE_GAME_DBJ_PLATFORM_H

// Everything this program knows about the operating system it is running
// on, in one struct. No raylib below this line -- the windowless test
// binary links this file, so anything that opens a window cannot live here.
#include <stdbool.h>
#include <stddef.h>

typedef struct dbj_platform dbj_platform;

struct dbj_platform {
	// Directory holding the running executable, no trailing separator.
	// Returns false on failure or when cap is too small; out is left
	// untouched in that case, never half-written.
	bool (*exe_dir)(char *out, size_t cap);

	// Directory separator this platform writes. Both characters work on
	// Windows, but a path shown in an error message should look native.
	char sep;
};

dbj_platform dbj_platform_make(void);

#endif  // DBJ_THE_GAME_DBJ_PLATFORM_H
