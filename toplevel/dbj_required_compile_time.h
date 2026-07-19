
#ifndef REQUIRED_COMPILETIME_H
#define REQUIRED_COMPILETIME_H

/*

Code in this repo requires GCC 15 or better
------------------------------------------
gcc.exe (MinGW-W64 x86_64-msvcrt-posix-seh, built by Brecht Sanders, r1) 15.2.0
Copyright (C) 2025 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

Clang is rejected even though it defines __GNUC__ for compatibility:
toplevel/dbj_defer.h relies on GNU nested functions ([[gnu::cleanup]] plus
an `auto void F(int*)` local function), which Clang has never
implemented and does not plan to.

*/

#if defined(__clang__)
#error "This repo requires GCC 15 or better -- Clang is not supported"
#elif !defined(__GNUC__) || __GNUC__ < 15
#error "This repo requires GCC 15 or better"
#endif

#define DBJ_CODING_SKILLS_OK_COMPILER (1 == 1)

#endif // REQUIRED_COMPILETIME_H
