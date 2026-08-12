#include "dbj_platform.h"

#include <string.h>

// The two implementations below answer the same question -- "where is the
// file I am running from" -- and the answer is the only thing that differs
// per OS. Keeping the #ifdef here, and nowhere else, is the whole point of
// this file: every caller above sees one struct with one function pointer.
#ifdef _WIN32

#include <windows.h>

static bool win_exe_dir(char *out, size_t cap)
{
	if (out == NULL || cap == 0) return false;

	char full[MAX_PATH] = {0};
	// Returns the length written. On truncation it returns cap and, before
	// Windows 10 1607, leaves the buffer unterminated -- so a return equal
	// to the buffer size is a failure, not a full success.
	DWORD written = GetModuleFileNameA(NULL, full, (DWORD)sizeof full);
	if (written == 0 || written >= sizeof full) return false;

	char *last = strrchr(full, '\\');
	if (last == NULL) last = strrchr(full, '/');
	// A module path with no separator at all should not happen, but the
	// answer if it does is "I do not know", not "the current directory".
	if (last == NULL) return false;

	size_t dir_len = (size_t)(last - full);
	if (dir_len + 1 > cap) return false;

	memcpy(out, full, dir_len);
	out[dir_len] = '\0';
	return true;
}

dbj_platform dbj_platform_make(void)
{
	return (dbj_platform){.exe_dir = win_exe_dir, .sep = '\\'};
}

#else

#include <unistd.h>

static bool posix_exe_dir(char *out, size_t cap)
{
	if (out == NULL || cap == 0) return false;

	char full[4096] = {0};
	// readlink does not terminate what it writes, and reports the byte
	// count it wrote; filling the buffer means the real path was longer.
	ssize_t written = readlink("/proc/self/exe", full, sizeof full - 1);
	if (written <= 0 || (size_t)written >= sizeof full - 1) return false;
	full[written] = '\0';

	char *last = strrchr(full, '/');
	if (last == NULL) return false;

	size_t dir_len = (size_t)(last - full);
	if (dir_len + 1 > cap) return false;

	memcpy(out, full, dir_len);
	out[dir_len] = '\0';
	return true;
}

dbj_platform dbj_platform_make(void)
{
	return (dbj_platform){.exe_dir = posix_exe_dir, .sep = '/'};
}

#endif
