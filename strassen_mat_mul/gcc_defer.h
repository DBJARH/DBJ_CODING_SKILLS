#pragma once

/*
 * Polyfill for the `defer` keyword (ISO/DIS TS 25755) using GCC nested
 * functions and the gnu::cleanup attribute — a technique by Jens Gustedt.
 *
 * Do NOT include this header when the compiler provides `defer` natively.
 * GCC 16+ does so when compiled with -fdefer-ts, signalled by the macro
 * __STDC_DEFER_TS25755__.  strassen.c guards the include accordingly:
 *
 *   #ifndef __STDC_DEFER_TS25755__
 *   #  include "gcc_defer.h"
 *   #endif
 *
 * Requires GCC with GNU extensions enabled (-std=gnu23 or similar).
 * MSVC/IntelliSense will reject `auto void F(int*)` — those are false
 * positives; this header is GCC-only.
 */

// Gustedt's defer macro
#define defer __DEFER(__COUNTER__)
#define __DEFER(N) __DEFER_(N)
#define __DEFER_(N) __DEFER__(__DEFER_FUNCTION_##N, __DEFER_VARIABLE_##N)

#define __DEFER__(F, V)        \
    auto void F(int*);         \
    [[gnu::cleanup(F)]] int V; \
    auto void F(int*)
