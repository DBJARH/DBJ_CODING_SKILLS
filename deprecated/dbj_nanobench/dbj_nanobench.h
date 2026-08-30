
/* dbj_nanobench.h — single-header C23 microbenchmark harness
 * gcc/clang, POSIX clock_gettime. Zero deps.
 *
 * STB-style single header: declarations are always visible; the
 * implementation is compiled only where DBJ_NANOBENCH_IMPLEMENTATION
 * is defined before this include. Exactly one .c file per binary
 * should do that — every caller in this repo builds one .c file per
 * binary, so define it right before this include in that file:
 *
 *     #define DBJ_NANOBENCH_IMPLEMENTATION
 *     #include "dbj_nanobench.h"
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

/* --- types --- */
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

/* --- declarations --- */
uint64_t DBJ_NB_now_ns(void);
DBJ_NB_scaled DBJ_NB_scale_ns(double ns);
void DBJ_NB_report(const DBJ_NB_result *r);

/* --- public front macros ---
 * DBJ_BENCH(name, val_type, { ...code assigning DBJ_NB_val... });
 * DBJ_BENCH_N(name, val_type, warmup_n, iters_n, { ... });
 * DBJ_MEASURE(name, { ...code, no assignment needed... });
 * DBJ_MEASURE_N(name, warmup_n, iters_n, { ... });
 *
 * DBJ_BENCH/DBJ_BENCH_N are for benchmarking an expression whose result
 * must survive to be DNO'd (kept from being optimized away) — DBJ_NB_val
 * is the out-parameter: assign the benchmarked expression's result to it
 * inside the block.
 *
 * DBJ_MEASURE/DBJ_MEASURE_N are for timing a void/side-effecting call —
 * no DBJ_NB_val, no DNO. Use these when there is nothing to assign.
 *
 * warmup_n/iters_n default to DBJ_NB_DEFAULT_WARMUP_ITERS/
 * DBJ_NB_DEFAULT_TIMED_ITERS in the non-_N forms.
 */
#define DBJ_NB_DEFAULT_WARMUP_ITERS 1000
#define DBJ_NB_DEFAULT_TIMED_ITERS 100000

#define DBJ_BENCH(_name, _val_type, _block) \
    DBJ_NB_BENCH(_name, DBJ_NB_DEFAULT_WARMUP_ITERS, DBJ_NB_DEFAULT_TIMED_ITERS, _val_type, _block)
#define DBJ_BENCH_N(_name, _val_type, _warmup, _iters, _block) \
    DBJ_NB_BENCH(_name, _warmup, _iters, _val_type, _block)

#define DBJ_MEASURE(_name, _block) \
    DBJ_NB_MEASURE(_name, DBJ_NB_DEFAULT_WARMUP_ITERS, DBJ_NB_DEFAULT_TIMED_ITERS, _block)
#define DBJ_MEASURE_N(_name, _warmup, _iters, _block) \
    DBJ_NB_MEASURE(_name, _warmup, _iters, _block)

/* --- implementation macros ---
 * VAL must be assigned inside the DBJ_NB_BENCH block — it gets DNO'd
 * automatically. DBJ_NB_MEASURE has no VAL and no DNO.
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

#define DBJ_NB_MEASURE(_name, _warmup, _iters, _block)                                                                \
    do                                                                                                                \
    {                                                                                                                 \
        DBJ_NB_result DBJ_NB_r = {.name = _name, .warmup_iters = (_warmup), .iters = (_iters), .min_ns = UINT64_MAX}; \
        for (uint64_t DBJ_NB_i = 0; DBJ_NB_i < DBJ_NB_r.warmup_iters; DBJ_NB_i++)                                     \
        {                                                                                                             \
            _block                                                                                                    \
        }                                                                                                             \
        for (uint64_t DBJ_NB_i = 0; DBJ_NB_i < DBJ_NB_r.iters; DBJ_NB_i++)                                            \
        {                                                                                                             \
            uint64_t DBJ_NB_t0 = DBJ_NB_now_ns();                                                                     \
            _block                                                                                                    \
            uint64_t DBJ_NB_dt = DBJ_NB_now_ns() - DBJ_NB_t0;                                                         \
            DBJ_NB_r.total_ns += DBJ_NB_dt;                                                                           \
            if (DBJ_NB_dt < DBJ_NB_r.min_ns)                                                                          \
                DBJ_NB_r.min_ns = DBJ_NB_dt;                                                                          \
            if (DBJ_NB_dt > DBJ_NB_r.max_ns)                                                                          \
                DBJ_NB_r.max_ns = DBJ_NB_dt;                                                                          \
        }                                                                                                             \
        DBJ_NB_report(&DBJ_NB_r);                                                                                     \
    } while (0)

/* --- implementation --- */
#ifdef DBJ_NANOBENCH_IMPLEMENTATION

uint64_t DBJ_NB_now_ns(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t)t.tv_sec * 1000000000ull + (uint64_t)t.tv_nsec;
}

DBJ_NB_scaled DBJ_NB_scale_ns(double ns)
{
    if (ns >= DBJ_NB_NS_PER_MS)
        return (DBJ_NB_scaled){ .value = ns / DBJ_NB_NS_PER_MS, .unit = "ms" };
    if (ns >= DBJ_NB_NS_PER_US)
        return (DBJ_NB_scaled){ .value = ns / DBJ_NB_NS_PER_US, .unit = "us" };
    return (DBJ_NB_scaled){ .value = ns, .unit = "ns" };
}

void DBJ_NB_report(const DBJ_NB_result *r)
{
    /* one unit for the whole row, picked from max_ns, so avg/min/max are
     * directly comparable instead of each floating to its own unit */
    DBJ_NB_scaled unit_ = DBJ_NB_scale_ns((double)r->max_ns);
    double divisor_ = unit_.value != 0.0 ? (double)r->max_ns / unit_.value : 1.0;
    double avg_ = ((double)r->total_ns / (double)r->iters) / divisor_;
    double min_ = (double)r->min_ns / divisor_;
    double max_ = unit_.value;
    printf("%-24s  avg=%7.2f %-2s  min=%7.2f %-2s  max=%7.2f %-2s  (n=%llu)\n",
           r->name,
           avg_, unit_.unit,
           min_, unit_.unit,
           max_, unit_.unit,
           (unsigned long long)r->iters);
}

#endif // DBJ_NANOBENCH_IMPLEMENTATION
