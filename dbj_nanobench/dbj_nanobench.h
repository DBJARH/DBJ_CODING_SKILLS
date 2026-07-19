
/* dbj_nanobench.h — single-header C23 microbenchmark harness
 * gcc/clang, POSIX clock_gettime. Zero deps.
 */
#pragma once

/* must be defined before the first <time.h> include below — on
 * MinGW-w64 _GNU_SOURCE alone does not declare localtime_r, unlike
 * on glibc; guarded so callers that already set these (or pull in
 * dbj_common.h first) don't get a redefinition warning */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>

/* --- optimization barriers --- */
#define DBJ_NB_DNO(x) asm volatile("" : : "r,m"(x) : "memory")
#define DBJ_NB_CLOBBER() asm volatile("" : : : "memory")

/* --- timing --- */
static inline uint64_t DBJ_NB_now_ns(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t)t.tv_sec * 1000000000ull + (uint64_t)t.tv_nsec;
}

typedef struct
{
    const char *name;
    uint64_t warmup_iters;
    uint64_t iters;
    uint64_t min_ns, max_ns, total_ns;
} DBJ_NB_result;

#define DBJ_NB_NS_PER_US 1000.0
#define DBJ_NB_NS_PER_MS 1000000.0

typedef struct
{
    double value;
    const char *unit;
} DBJ_NB_scaled;

static inline DBJ_NB_scaled DBJ_NB_scale_ns(double ns)
{
    if (ns >= DBJ_NB_NS_PER_MS)
        return (DBJ_NB_scaled){ .value = ns / DBJ_NB_NS_PER_MS, .unit = "ms" };
    if (ns >= DBJ_NB_NS_PER_US)
        return (DBJ_NB_scaled){ .value = ns / DBJ_NB_NS_PER_US, .unit = "us" };
    return (DBJ_NB_scaled){ .value = ns, .unit = "ns" };
}

static inline void DBJ_NB_report(const DBJ_NB_result *r)
{
    DBJ_NB_scaled avg_ = DBJ_NB_scale_ns((double)r->total_ns / (double)r->iters);
    DBJ_NB_scaled min_ = DBJ_NB_scale_ns((double)r->min_ns);
    DBJ_NB_scaled max_ = DBJ_NB_scale_ns((double)r->max_ns);
    printf("%-24s  avg=%7.2f %-2s  min=%7.2f %-2s  max=%7.2f %-2s  (n=%llu)\n",
           r->name,
           avg_.value, avg_.unit,
           min_.value, min_.unit,
           max_.value, max_.unit,
           (unsigned long long)r->iters);
}

/* --- public front macro ---
 * DBJ_BENCH(name, val_type, { ...code assigning DBJ_NB_val... });
 * DBJ_BENCH(name, val_type, warmup_n, iters_n, { ... });
 *
 * DBJ_NB_val is the out-parameter: assign the benchmarked expression's
 * result to it inside the block so it can be DNO'd (kept from being
 * optimized away) without UB.
 *
 * warmup_n/iters_n default to DBJ_NB_DEFAULT_WARMUP_ITERS/
 * DBJ_NB_DEFAULT_TIMED_ITERS when omitted.
 */
#define DBJ_NB_DEFAULT_WARMUP_ITERS 1000
#define DBJ_NB_DEFAULT_TIMED_ITERS 100000

#define DBJ_BENCH(...) DBJ_BENCH_DISPATCH(__VA_ARGS__, DBJ_BENCH5, DBJ_BENCH4, DBJ_BENCH3)(__VA_ARGS__)
#define DBJ_BENCH_DISPATCH(_1, _2, _3, _4, _5, _NAME, ...) _NAME
#define DBJ_BENCH3(_name, _val_type, _block) \
    DBJ_NB_BENCH(_name, DBJ_NB_DEFAULT_WARMUP_ITERS, DBJ_NB_DEFAULT_TIMED_ITERS, _val_type, _block)
#define DBJ_BENCH4(_name, _val_type, _iters, _block) \
    DBJ_NB_BENCH(_name, DBJ_NB_DEFAULT_WARMUP_ITERS, _iters, _val_type, _block)
#define DBJ_BENCH5(_name, _val_type, _warmup, _iters, _block) \
    DBJ_NB_BENCH(_name, _warmup, _iters, _val_type, _block)

/* --- implementation macro ---
 * BENCH(name, warmup_n, iters_n, { ...code using VAL... });
 * VAL must be assigned inside the block — it gets DNO'd automatically.
 */
#define DBJ_NB_BENCH(_name, _warmup, _iters, _val_type, _block)                                                       \
    do                                                                                                                \
    {                                                                                                                 \
        _val_type DBJ_NB_val;                                                                                         \
        DBJ_NB_result DBJ_NB_r = {.name = _name, .warmup_iters = (_warmup), .iters = (_iters), .min_ns = UINT64_MAX}; \
        for (uint64_t DBJ_NB_i = 0; DBJ_NB_i < DBJ_NB_r.warmup_iters; DBJ_NB_i++)                                     \
        {                                                                                                             \
            _block                                                                                                    \
                DBJ_NB_DNO(DBJ_NB_val);                                                                               \
        }                                                                                                             \
        for (uint64_t DBJ_NB_i = 0; DBJ_NB_i < DBJ_NB_r.iters; DBJ_NB_i++)                                            \
        {                                                                                                             \
            uint64_t DBJ_NB_t0 = DBJ_NB_now_ns();                                                                     \
            _block                                                                                                    \
                DBJ_NB_DNO(DBJ_NB_val);                                                                               \
            uint64_t DBJ_NB_dt = DBJ_NB_now_ns() - DBJ_NB_t0;                                                         \
            DBJ_NB_r.total_ns += DBJ_NB_dt;                                                                           \
            if (DBJ_NB_dt < DBJ_NB_r.min_ns)                                                                          \
                DBJ_NB_r.min_ns = DBJ_NB_dt;                                                                          \
            if (DBJ_NB_dt > DBJ_NB_r.max_ns)                                                                          \
                DBJ_NB_r.max_ns = DBJ_NB_dt;                                                                          \
        }                                                                                                             \
        DBJ_NB_report(&DBJ_NB_r);                                                                                     \
    } while (0)

#ifdef DBJ_NANOBENCH_SMOKE_TEST

static bool DBJ_NB_is_prime(long n)
{
    if (n < 2)
        return false;
    for (long d = 2; d * d <= n; d++)
        if (n % d == 0)
            return false;
    return true;
}

static long nth_prime(int n)
{
    long count = 0, candidate = 1;
    while (count < n)
    {
        candidate++;
        if (DBJ_NB_is_prime(candidate))
            count++;
    }
    return candidate;
}

#define DBJ_NB_SMOKE_PRIME_INDEX 1000

static int dbj_nanobench_smoke_test(char *argv[static 1])
{
    (void)argv;
    DBJ_BENCH("nth_prime(1000) defaults", long, {
        DBJ_NB_val = nth_prime(DBJ_NB_SMOKE_PRIME_INDEX);
    });

    // full form: name, val_type, warmup_iters, timed_iters, block
    // DBJ_BENCH("nth_prime(1000)", long, DBJ_NB_DEFAULT_WARMUP_ITERS, DBJ_NB_DEFAULT_TIMED_ITERS, {
    //     DBJ_NB_val = nth_prime(DBJ_NB_SMOKE_PRIME_INDEX);
    // });
    
    return 0;
}

#endif // DBJ_NANOBENCH_SMOKE_TEST
