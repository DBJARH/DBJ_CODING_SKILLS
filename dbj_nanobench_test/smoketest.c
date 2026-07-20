/*
    Compile from the repo root (needs -I toplevel to resolve
    <dbj_nanobench.h>, <dbj_str.h> and <dbj_nth_prime.h>):

    gcc -std=gnu23 -O2 -Wall -Wextra -Itoplevel -o dbj_nanobench_test/smoketest.exe dbj_nanobench_test/smoketest.c
*/
#define DBJ_NANOBENCH_IMPLEMENTATION
#include <dbj_nanobench.h>
#include <dbj_str.h>
#include <dbj_nth_prime.h>
#include <stdio.h>

DEFINE_DBJSTR_TYPE(str80, 80)

#define SMOKE_PRIME_INDEX 1000

/* benchmark dbj_str's create path: format nth_prime's result into a
 * stack buffer and hand it to str80_create each iteration, instead of
 * just computing the number. */
static void bench_dbj_str_fmt(void) {
    DBJ_BENCH("nth_prime(1000) -> str80 (create)", str80, {
        long p_ = dbj_get_nth_prime(1000);
        unsigned char buf[80];
        snprintf((char *)buf, sizeof(buf), "nth_prime(1000) = %ld", p_);
        DBJ_NB_val = str80_create(buf);
    });
}

int main(const int argc, char * argv[ static argc + 1] ) {
    (void)argv;

    DBJ_BENCH("nth_prime(1000) defaults", long, {
        DBJ_NB_val = dbj_get_nth_prime(SMOKE_PRIME_INDEX);
    });

    // void/side-effecting call — no DBJ_NB_val to assign
    DBJ_MEASURE("nth_prime(1000) side-effect only", {
        (void)dbj_get_nth_prime(SMOKE_PRIME_INDEX);
    });

    bench_dbj_str_fmt();
    return 0;
}


// Notes:
// - `_val_type` is required so `DBJ_NB_val` has correct storage — avoids UB from an untyped barrier.
// - Warmup phase runs the block but discards timing, per ubench's approach.
// - min/avg/max, not just avg — surfaces branch-predictor/cache noise instead of hiding it.
// - No malloc, no dependency, compiles as one TU.

// Possible extension: A variant that also samples `rdtsc` directly for sub-10ns ops, where `clock_gettime` overhead itself would dominate?