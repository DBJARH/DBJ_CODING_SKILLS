
#define DBJ_NANOBENCH_SMOKE_TEST
#include "dbj_nanobench.h"
#include "../dbj_str_2026/dbj_common.h"

/* benchmark dbj_str_2026's formatted assign: format nth_prime's
 * result into a stack str_80 each iteration, instead of just
 * computing the number. */
static void bench_dbj_str_fmt(void) {
    DBJ_BENCH("nth_prime(1000) -> str_80 (DBJ_STR_FMT)", str_80, {
        long p_ = nth_prime(1000);
        DBJ_STR_FMT(&DBJ_NB_val, "nth_prime(1000) = %ld", p_);
    });
}

int main(const int argc, char * argv[ static argc + 1] ) {

    int rc_ = dbj_nanobench_smoke_test(argv);
    bench_dbj_str_fmt();
    return rc_;
}


// Notes:
// - `_val_type` is required so `DBJ_NB_val` has correct storage — avoids UB from an untyped barrier.
// - Warmup phase runs the block but discards timing, per ubench's approach.
// - min/avg/max, not just avg — surfaces branch-predictor/cache noise instead of hiding it.
// - No malloc, no dependency, compiles as one TU.

// Possible extension: A variant that also samples `rdtsc` directly for sub-10ns ops, where `clock_gettime` overhead itself would dominate?