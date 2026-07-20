#pragma once

/* GCC statement-expression: single-eval, usable as an expression. */
#define DBJ_IS_PRIME(x_) \
    ({ \
        const int x = (x_); \
        int result = (x >= 2); \
        for (int d = 2; result && (d * d) <= x; d++) { \
            if (0 == (x % d)) result = 0; \
        } \
        result; \
    })

/* 1-indexed: get_nth_prime(1) == 2, get_nth_prime(2) == 3, ... */
static int dbj_get_nth_prime(const int n) {
    int count = 0;
    int candidate = 1;
    while (count < n) {
        candidate++;
        if (DBJ_IS_PRIME(candidate)) {
            count++;
        }
    }
    return candidate;
}