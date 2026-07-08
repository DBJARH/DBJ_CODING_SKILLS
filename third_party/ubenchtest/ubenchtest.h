/*
   ubenchtest.h -- GCC-only benchmarking header built from sheredom's
   public-domain ubench.h, with the assertion macros from his utest.h
   folded in for use inside benchmarks:

       https://github.com/sheredom/ubench.h   (microbenchmarking)
       https://github.com/sheredom/utest.h    (assertion macros only)

   There is only one kind of unit of work here: UBENCH(). There is no
   UTEST() and no test-registration/filtering/sorting/XML-report
   machinery -- none of that survived the merge. What did survive is
   utest's assertion macros (ASSERT_EQ, EXPECT_TRUE, ASSERT_STREQ, ...)
   so a UBENCH() body can sanity-check the result it just measured
   without hand-rolling comparisons.

   Trimmed for this repo's own use: GCC 15+ on Linux/Windows(MinGW)
   only. Removed entirely, not just #ifdef'd out: MSVC (.CRT$XCU /
   __declspec(allocate) constructor trick, QueryPerformanceCounter,
   /Wxxxx pragmas), Clang (__has_warning-guarded pragmas), C++
   (template type_deducer, constructor-struct INITIALIZER), TinyCC,
   Emscripten, BSD/Solaris/Haiku clock paths. Original upstream files
   are public domain (Unlicense); this trimmed derivative carries the
   same terms.

   Using assertions inside a UBENCH() body: every assertion macro
   expands to a write through a pointer literally named utest_result.
   Declare ENABLE_UBNCH_ASSERT_MACROS() as the first statement in a
   UBENCH() body to bring that pointer into scope. Optionally check the
   outcome at the end with UBENCH_ASSERT_SCOPE_CHECK(), which fails the
   benchmark via UBENCH_FAIL() if any assertion in the body failed --
   if that call is omitted, a failing assertion just prints its failure
   line and the benchmark still reports OK.

   Use the non-fatal EXPECT_* family (EXPECT_EQ, EXPECT_TRUE, ...)
   inside a UBENCH() body, not the fatal ASSERT_* family. ASSERT_*
   does an early return on failure, same as it would in a unit test --
   but UBENCH_DO_BENCHMARK() (hidden inside the UBENCH() macro) calls
   the body hundreds of times to gather samples for the confidence
   interval, so a failing ASSERT_* reprints its failure message once
   per sample instead of once, and UBENCH_ASSERT_SCOPE_CHECK() (if
   used) never gets a chance to run. EXPECT_* records the failure and
   lets the body finish normally, so it plays correctly with the
   sample loop -- though note that without UBENCH_ASSERT_SCOPE_CHECK(),
   a failing EXPECT_* also reprints its failure line once per sample,
   since nothing stops the loop either way.

   Do not put the ubench measurement loop itself inside what is being
   asserted on -- write the assertion to check the outcome of one
   iteration of work, not to drive a loop of its own.

   Ordering: ubench runs benchmarks in registration order (plain array
   walk, no sort) -- but registration order is whatever order the
   __attribute__((constructor)) functions actually run in, which is
   NOT source declaration order. Verified on GCC 15 / MinGW-w64: it is
   the reverse of declaration order in this file. Do not rely on
   declaration order between separate UBENCH() blocks; if benchmarks
   must run in a specific sequence, sequence them by hand inside one
   UBENCH() (fixtures, UBENCH_F, may offer a better answer here --
   still open, see project memory).
*/
#ifndef DBJ_UBENCHTEST_H_INCLUDED
#define DBJ_UBENCHTEST_H_INCLUDED

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <inttypes.h>
#include <unistd.h>
#include <time.h>

/* ======================================================================
   shared helpers
   ====================================================================== */

typedef int64_t  dbj_ubt_int64_t;
typedef uint64_t dbj_ubt_uint64_t;
typedef uint32_t dbj_ubt_uint32_t;

#define DBJ_UBT_PRId64 PRId64
#define DBJ_UBT_PRIu64 PRIu64

#define DBJ_UBT_COLOUR_OUTPUT() (isatty(STDOUT_FILENO))

static inline dbj_ubt_int64_t dbj_ubt_mul_div(const dbj_ubt_int64_t value,
                                               const dbj_ubt_int64_t numer,
                                               const dbj_ubt_int64_t denom) {
    const dbj_ubt_int64_t q = value / denom;
    const dbj_ubt_int64_t r = value % denom;
    return q * numer + r * numer / denom;
}

static inline dbj_ubt_int64_t dbj_ubt_ns(void) {
    struct timespec ts = {0, 0};
    clock_gettime(CLOCK_REALTIME, &ts);
    return (dbj_ubt_int64_t)ts.tv_sec * 1000 * 1000 * 1000 + ts.tv_nsec;
}

static inline int dbj_ubt_strncmp(const char *a, const char *b, size_t n) {
    /* avoid libc strncmp -- upstream note: trips -Wall/-Werror on gcc/clang */
    unsigned i;
    for (i = 0; i < n; i++) {
        if (a[i] < b[i]) return -1;
        if (a[i] > b[i]) return 1;
    }
    return 0;
}

static inline int dbj_ubt_should_filter(const char *filter, const char *name) {
    if (filter) {
        const char *filter_cur = filter;
        const char *name_cur = name;
        const char *filter_wildcard = NULL;

        while (('\0' != *filter_cur) && ('\0' != *name_cur)) {
            if ('*' == *filter_cur) {
                filter_wildcard = filter_cur;
                filter_cur++;

                while (('\0' != *filter_cur) && ('\0' != *name_cur)) {
                    if ('*' == *filter_cur) {
                        break;
                    } else if (*filter_cur != *name_cur) {
                        filter_cur = filter_wildcard;
                    }
                    name_cur++;
                    filter_cur++;
                }

                if (('\0' == *filter_cur) && ('\0' == *name_cur)) return 0;
                if ('\0' == *name_cur) return 1;
            } else {
                if (*name_cur != *filter_cur) return 1;
                name_cur++;
                filter_cur++;
            }
        }

        if (('\0' != *filter_cur) ||
            (('\0' != *name_cur) &&
             ((filter == filter_cur) || ('*' != filter_cur[-1])))) {
            return 1;
        }
    }
    return 0;
}

/* ======================================================================
   ubench state -- declared ahead of the assertion macros below because
   they print through ubench_state.output
   ====================================================================== */

#define UBENCH_INITIALIZER(f)                                                 \
    static void f(void) __attribute__((constructor));                        \
    static void f(void)

struct ubench_run_state_s {
    dbj_ubt_int64_t *ns;
    dbj_ubt_int64_t size;
    dbj_ubt_int64_t sample;
};

typedef void (*ubench_benchmark_t)(struct ubench_run_state_s *ubs);

struct ubench_benchmark_state_s {
    ubench_benchmark_t func;
    char *name;
};

enum ubench_result_e { UBENCH_RESULT_PASS, UBENCH_RESULT_SKIP, UBENCH_RESULT_FAIL };

struct ubench_state_s {
    struct ubench_benchmark_state_s *benchmarks;
    size_t benchmarks_length;
    FILE *output;
    double confidence;
    enum ubench_result_e result;
};

extern struct ubench_state_s ubench_state;

#define UBENCH_PRINTF(...)                                                    \
    if (ubench_state.output) {                                               \
        fprintf(ubench_state.output, __VA_ARGS__);                           \
    }                                                                        \
    printf(__VA_ARGS__)

/* ======================================================================
   assertion macros (ASSERT_*, EXPECT_*) -- from utest.h, minus every
   piece of test-registration machinery. Usable inside a UBENCH() body
   after ENABLE_UBNCH_ASSERT_MACROS().

   Optional: #define DBJ_UBENCHTEST_NO_ASSERT before #including this
   header to drop this whole section for a TU that has no need of it.
   ====================================================================== */
#ifndef DBJ_UBENCHTEST_NO_ASSERT

#define UTEST_TEST_PASSED (0)
#define UTEST_TEST_FAILURE (1)

/* first statement in a UBENCH() body that wants to use the assertion
   macros: brings a correctly-named (int *utest_result) into scope, so
   the assertion macros have something to write through. Pair with
   UBENCH_ASSERT_SCOPE_CHECK() if a failed assertion should also fail
   the benchmark; otherwise a failing EXPECT_* just prints its failure
   line and the benchmark still reports OK. */
#define ENABLE_UBNCH_ASSERT_MACROS()                                          \
    int dbj_ubt_assert_result = UTEST_TEST_PASSED;                           \
    int *const utest_result = &dbj_ubt_assert_result

#define UBENCH_ASSERT_SCOPE_CHECK()                                           \
    do {                                                                     \
        if (UTEST_TEST_PASSED != dbj_ubt_assert_result) {                    \
            UBENCH_FAIL();                                                   \
        }                                                                    \
    } while (0)

#define utest_type_printer(val)                                               \
    UBENCH_PRINTF(                                                            \
        _Generic((val),                                                      \
        signed char: "%d",                                                   \
        unsigned char: "%u",                                                 \
        short: "%d",                                                         \
        unsigned short: "%u",                                                \
        int: "%d",                                                           \
        long: "%ld",                                                         \
        long long: "%lld",                                                   \
        unsigned: "%u",                                                      \
        unsigned long: "%lu",                                                \
        unsigned long long: "%llu",                                          \
        float: "%f",                                                         \
        double: "%f",                                                        \
        long double: "%Lf",                                                  \
        default: _Generic((val - val), ptrdiff_t: "%p", default: "undef")),  \
        (val))

#define UTEST_AUTO(x) __typeof__(x + 0)

#define UTEST_COND(x, y, cond, msg, is_assert)                                \
    do {                                                                     \
        UTEST_AUTO(x) xEval = (x);                                           \
        UTEST_AUTO(y) yEval = (y);                                           \
        if (!((xEval)cond(yEval))) {                                         \
            const char *const xAsString = #x;                               \
            const char *const yAsString = #y;                               \
            UBENCH_PRINTF("%s:%i: Failure\n", __FILE__, __LINE__);           \
            UBENCH_PRINTF("  Expected : (");                                 \
            UBENCH_PRINTF("%s) " #cond " (%s", xAsString, yAsString);        \
            UBENCH_PRINTF(")\n");                                            \
            UBENCH_PRINTF("    Actual : ");                                  \
            utest_type_printer(xEval);                                       \
            UBENCH_PRINTF(" vs ");                                           \
            utest_type_printer(yEval);                                       \
            UBENCH_PRINTF("\n");                                             \
            if (strlen(msg) > 0) {                                           \
                UBENCH_PRINTF("   Message : %s\n", msg);                     \
            }                                                                \
            *utest_result = UTEST_TEST_FAILURE;                              \
            if (is_assert) {                                                 \
                return;                                                      \
            }                                                                \
        }                                                                    \
    } while (0)

#define EXPECT_EQ(x, y) UTEST_COND(x, y, ==, "", 0)
#define EXPECT_EQ_MSG(x, y, msg) UTEST_COND(x, y, ==, msg, 0)
#define ASSERT_EQ(x, y) UTEST_COND(x, y, ==, "", 1)
#define ASSERT_EQ_MSG(x, y, msg) UTEST_COND(x, y, ==, msg, 1)

#define EXPECT_NE(x, y) UTEST_COND(x, y, !=, "", 0)
#define EXPECT_NE_MSG(x, y, msg) UTEST_COND(x, y, !=, msg, 0)
#define ASSERT_NE(x, y) UTEST_COND(x, y, !=, "", 1)
#define ASSERT_NE_MSG(x, y, msg) UTEST_COND(x, y, !=, msg, 1)

#define EXPECT_LT(x, y) UTEST_COND(x, y, <, "", 0)
#define EXPECT_LT_MSG(x, y, msg) UTEST_COND(x, y, <, msg, 0)
#define ASSERT_LT(x, y) UTEST_COND(x, y, <, "", 1)
#define ASSERT_LT_MSG(x, y, msg) UTEST_COND(x, y, <, msg, 1)

#define EXPECT_LE(x, y) UTEST_COND(x, y, <=, "", 0)
#define EXPECT_LE_MSG(x, y, msg) UTEST_COND(x, y, <=, msg, 0)
#define ASSERT_LE(x, y) UTEST_COND(x, y, <=, "", 1)
#define ASSERT_LE_MSG(x, y, msg) UTEST_COND(x, y, <=, msg, 1)

#define EXPECT_GT(x, y) UTEST_COND(x, y, >, "", 0)
#define EXPECT_GT_MSG(x, y, msg) UTEST_COND(x, y, >, msg, 0)
#define ASSERT_GT(x, y) UTEST_COND(x, y, >, "", 1)
#define ASSERT_GT_MSG(x, y, msg) UTEST_COND(x, y, >, msg, 1)

#define EXPECT_GE(x, y) UTEST_COND(x, y, >=, "", 0)
#define EXPECT_GE_MSG(x, y, msg) UTEST_COND(x, y, >=, msg, 0)
#define ASSERT_GE(x, y) UTEST_COND(x, y, >=, "", 1)
#define ASSERT_GE_MSG(x, y, msg) UTEST_COND(x, y, >=, msg, 1)

#define UTEST_TRUE(x, msg, is_assert)                                         \
    do {                                                                     \
        const int xEval = !!(x);                                            \
        if (!(xEval)) {                                                     \
            UBENCH_PRINTF("%s:%i: Failure\n", __FILE__, __LINE__);           \
            UBENCH_PRINTF("  Expected : true\n");                            \
            UBENCH_PRINTF("    Actual : %s\n", (xEval) ? "true" : "false");  \
            if (strlen(msg) > 0) {                                           \
                UBENCH_PRINTF("   Message : %s\n", msg);                     \
            }                                                                \
            *utest_result = UTEST_TEST_FAILURE;                              \
            if (is_assert) {                                                 \
                return;                                                      \
            }                                                                \
        }                                                                    \
    } while (0)

#define EXPECT_TRUE(x) UTEST_TRUE(x, "", 0)
#define EXPECT_TRUE_MSG(x, msg) UTEST_TRUE(x, msg, 0)
#define ASSERT_TRUE(x) UTEST_TRUE(x, "", 1)
#define ASSERT_TRUE_MSG(x, msg) UTEST_TRUE(x, msg, 1)

#define UTEST_FALSE(x, msg, is_assert)                                        \
    do {                                                                     \
        const int xEval = !!(x);                                            \
        if (xEval) {                                                        \
            UBENCH_PRINTF("%s:%i: Failure\n", __FILE__, __LINE__);           \
            UBENCH_PRINTF("  Expected : false\n");                           \
            UBENCH_PRINTF("    Actual : %s\n", (xEval) ? "true" : "false");  \
            if (strlen(msg) > 0) {                                           \
                UBENCH_PRINTF("   Message : %s\n", msg);                     \
            }                                                                \
            *utest_result = UTEST_TEST_FAILURE;                              \
            if (is_assert) {                                                 \
                return;                                                      \
            }                                                                \
        }                                                                    \
    } while (0)

#define EXPECT_FALSE(x) UTEST_FALSE(x, "", 0)
#define EXPECT_FALSE_MSG(x, msg) UTEST_FALSE(x, msg, 0)
#define ASSERT_FALSE(x) UTEST_FALSE(x, "", 1)
#define ASSERT_FALSE_MSG(x, msg) UTEST_FALSE(x, msg, 1)

#define UTEST_STREQ(x, y, msg, is_assert)                                     \
    do {                                                                     \
        if (0 != strcmp((x), (y))) {                                        \
            UBENCH_PRINTF("%s:%i: Failure\n", __FILE__, __LINE__);           \
            UBENCH_PRINTF("  Expected : \"%s\"\n", (x));                     \
            UBENCH_PRINTF("    Actual : \"%s\"\n", (y));                     \
            if (strlen(msg) > 0) {                                           \
                UBENCH_PRINTF("   Message : %s\n", msg);                     \
            }                                                                \
            *utest_result = UTEST_TEST_FAILURE;                              \
            if (is_assert) {                                                 \
                return;                                                      \
            }                                                                \
        }                                                                    \
    } while (0)

#define EXPECT_STREQ(x, y) UTEST_STREQ(x, y, "", 0)
#define EXPECT_STREQ_MSG(x, y, msg) UTEST_STREQ(x, y, msg, 0)
#define ASSERT_STREQ(x, y) UTEST_STREQ(x, y, "", 1)
#define ASSERT_STREQ_MSG(x, y, msg) UTEST_STREQ(x, y, msg, 1)

#endif /* !DBJ_UBENCHTEST_NO_ASSERT */

/* ======================================================================
   ubench -- benchmark definition and runner (UBENCH_*)
   ====================================================================== */

#define UBENCH_DO_BENCHMARK() while (ubench_do_benchmark(ubench_run_state) > 0)

#define UBENCH_SKIP()                                                         \
    do {                                                                     \
        UBENCH_PRINTF("%s:%i: Skipped\n", __FILE__, __LINE__);               \
        ubench_state.result = UBENCH_RESULT_SKIP;                            \
        return;                                                              \
    } while (0)

#define UBENCH_FAIL()                                                         \
    do {                                                                     \
        UBENCH_PRINTF("%s:%i: Failure\n", __FILE__, __LINE__);               \
        ubench_state.result = UBENCH_RESULT_FAIL;                            \
        return;                                                              \
    } while (0)

#define UBENCH_EX(SET, NAME)                                                  \
    extern struct ubench_state_s ubench_state;                               \
    static void ubench_##SET##_##NAME(struct ubench_run_state_s *ubs);       \
    UBENCH_INITIALIZER(ubench_register_##SET##_##NAME) {                     \
        const size_t index = ubench_state.benchmarks_length++;               \
        const char name_part[] = #SET "." #NAME;                            \
        const size_t name_size = strlen(name_part) + 1;                     \
        char *name = (char *)malloc(name_size);                            \
        ubench_state.benchmarks = (struct ubench_benchmark_state_s *)realloc(\
            (void *)ubench_state.benchmarks,                               \
            sizeof(struct ubench_benchmark_state_s) * ubench_state.benchmarks_length); \
        ubench_state.benchmarks[index].func = &ubench_##SET##_##NAME;        \
        ubench_state.benchmarks[index].name = name;                         \
        snprintf(name, name_size, "%s", name_part);                        \
    }                                                                       \
    void ubench_##SET##_##NAME(struct ubench_run_state_s *ubench_run_state)

#define UBENCH(SET, NAME)                                                     \
    static void ubench_run_##SET##_##NAME(void);                            \
    UBENCH_EX(SET, NAME) {                                                   \
        UBENCH_DO_BENCHMARK() { ubench_run_##SET##_##NAME(); }               \
    }                                                                       \
    void ubench_run_##SET##_##NAME(void)

#define UBENCH_F_SETUP(FIXTURE)                                               \
    static void ubench_f_setup_##FIXTURE(struct FIXTURE *ubench_fixture)

#define UBENCH_F_TEARDOWN(FIXTURE)                                            \
    static void ubench_f_teardown_##FIXTURE(struct FIXTURE *ubench_fixture)

#define UBENCH_EX_F(FIXTURE, NAME)                                            \
    extern struct ubench_state_s ubench_state;                              \
    static void ubench_f_setup_##FIXTURE(struct FIXTURE *);                 \
    static void ubench_f_teardown_##FIXTURE(struct FIXTURE *);              \
    static void ubench_run_ex_##FIXTURE##_##NAME(struct FIXTURE *,          \
                                                  struct ubench_run_state_s *); \
    static void ubench_f_##FIXTURE##_##NAME(                                \
        struct ubench_run_state_s *ubench_run_state) {                      \
        struct FIXTURE fixture;                                             \
        memset(&fixture, 0, sizeof(fixture));                              \
        ubench_f_setup_##FIXTURE(&fixture);                                \
        ubench_run_ex_##FIXTURE##_##NAME(&fixture, ubench_run_state);      \
        ubench_f_teardown_##FIXTURE(&fixture);                             \
    }                                                                      \
    UBENCH_INITIALIZER(ubench_register_##FIXTURE##_##NAME) {                \
        const size_t index = ubench_state.benchmarks_length++;              \
        const char name_part[] = #FIXTURE "." #NAME;                       \
        const size_t name_size = strlen(name_part) + 1;                    \
        char *name = (char *)malloc(name_size);                           \
        ubench_state.benchmarks = (struct ubench_benchmark_state_s *)realloc(\
            (void *)ubench_state.benchmarks,                              \
            sizeof(struct ubench_benchmark_state_s) * ubench_state.benchmarks_length); \
        ubench_state.benchmarks[index].func = &ubench_f_##FIXTURE##_##NAME;  \
        ubench_state.benchmarks[index].name = name;                        \
        snprintf(name, name_size, "%s", name_part);                       \
    }                                                                      \
    void ubench_run_ex_##FIXTURE##_##NAME(                                  \
        struct FIXTURE *ubench_fixture, struct ubench_run_state_s *ubench_run_state)

#define UBENCH_F(FIXTURE, NAME)                                               \
    static void ubench_run_##FIXTURE##_##NAME(struct FIXTURE *);            \
    UBENCH_EX_F(FIXTURE, NAME) {                                            \
        UBENCH_DO_BENCHMARK() { ubench_run_##FIXTURE##_##NAME(ubench_fixture); } \
    }                                                                      \
    void ubench_run_##FIXTURE##_##NAME(struct FIXTURE *ubench_fixture)

static inline int ubench_do_benchmark(struct ubench_run_state_s *const ubs) {
    const dbj_ubt_int64_t curr_sample = ubs->sample++;
    ubs->ns[curr_sample] = dbj_ubt_ns();

    if (UBENCH_RESULT_SKIP == ubench_state.result ||
        UBENCH_RESULT_FAIL == ubench_state.result) {
        return 0;
    }
    return curr_sample < ubs->size ? 1 : 0;
}

static inline FILE *ubench_fopen(const char *filename, const char *mode) {
    return fopen(filename, mode);
}

static inline int ubench_main(int argc, const char *const argv[]);
int ubench_main(int argc, const char *const argv[]) {
    dbj_ubt_uint64_t failed = 0;
    size_t index = 0;
    size_t *failed_benchmarks = NULL;
    size_t failed_benchmarks_length = 0;
    const char *filter = NULL;
    dbj_ubt_uint64_t ran_benchmarks = 0;

    enum colours { RESET, GREEN, RED, YELLOW };
    const int use_colours = DBJ_UBT_COLOUR_OUTPUT();
    const char *colours[] = {"\033[0m", "\033[32m", "\033[31m", "\033[33m"};
    if (!use_colours) {
        for (index = 0; index < sizeof colours / sizeof colours[0]; index++) {
            colours[index] = "";
        }
    }

    for (index = 1; index < (size_t)argc; index++) {
        const char help_str[] = "--help";
        const char list_str[] = "--list-benchmarks";
        const char filter_str[] = "--filter=";
        const char output_str[] = "--output=";
        const char confidence_str[] = "--confidence=";

        if (0 == dbj_ubt_strncmp(argv[index], help_str, strlen(help_str))) {
            printf("ubenchtest.h - the benchmarking solution for C\n"
                   "Command line Options:\n"
                   "  --help                    Show this message and exit.\n"
                   "  --filter=<filter>         Filter the benchmarks to run.\n"
                   "  --list-benchmarks         List benchmarks, one per line.\n"
                   "  --output=<output>         Output a CSV file of the results.\n"
                   "  --confidence=<confidence> Confidence cut-off, default 2.5%%\n");
            goto cleanup;
        } else if (0 == dbj_ubt_strncmp(argv[index], filter_str, strlen(filter_str))) {
            filter = argv[index] + strlen(filter_str);
        } else if (0 == dbj_ubt_strncmp(argv[index], output_str, strlen(output_str))) {
            ubench_state.output = ubench_fopen(argv[index] + strlen(output_str), "w+");
        } else if (0 == dbj_ubt_strncmp(argv[index], list_str, strlen(list_str))) {
            for (index = 0; index < ubench_state.benchmarks_length; index++) {
                UBENCH_PRINTF("%s\n", ubench_state.benchmarks[index].name);
            }
            goto cleanup;
        } else if (0 == dbj_ubt_strncmp(argv[index], confidence_str, strlen(confidence_str))) {
            ubench_state.confidence = atof(argv[index] + strlen(confidence_str));
            if ((ubench_state.confidence < 0) || (ubench_state.confidence > 100)) {
                fprintf(stderr, "Confidence must be in [0..100] (you specified %f)\n",
                        ubench_state.confidence);
                goto cleanup;
            }
        }
    }

    for (index = 0; index < ubench_state.benchmarks_length; index++) {
        if (dbj_ubt_should_filter(filter, ubench_state.benchmarks[index].name)) continue;
        ran_benchmarks++;
    }

    printf("%s[==========]%s Running %" DBJ_UBT_PRIu64 " benchmarks.\n",
           colours[GREEN], colours[RESET], (dbj_ubt_uint64_t)ran_benchmarks);

    if (ubench_state.output) {
        fprintf(ubench_state.output, "name, mean (ns), stddev (%%), confidence (%%)\n");
    }

    for (index = 0; index < ubench_state.benchmarks_length; index++) {
        int result = 1;
        size_t mndex = 0;
        dbj_ubt_int64_t best_avg_ns = 0;
        double best_deviation = 0;
        double best_confidence = 101.0;
        struct ubench_run_state_s ubs;

#define DBJ_UBT_MIN_ITERATIONS 10
#define DBJ_UBT_MAX_ITERATIONS 500
        dbj_ubt_int64_t iterations = 10;
        const dbj_ubt_int64_t max_iterations = DBJ_UBT_MAX_ITERATIONS;
        const dbj_ubt_int64_t min_iterations = DBJ_UBT_MIN_ITERATIONS;
        dbj_ubt_int64_t ns[DBJ_UBT_MAX_ITERATIONS + 1];
#undef DBJ_UBT_MAX_ITERATIONS
#undef DBJ_UBT_MIN_ITERATIONS

        if (dbj_ubt_should_filter(filter, ubench_state.benchmarks[index].name)) continue;

        printf("%s[ RUN      ]%s %s\n", colours[GREEN], colours[RESET],
               ubench_state.benchmarks[index].name);

        ubs.ns = ns;
        ubs.size = 1;
        ubs.sample = 0;

        ubench_state.result = UBENCH_RESULT_PASS;
        ubench_state.benchmarks[index].func(&ubs);

        if (UBENCH_RESULT_SKIP == ubench_state.result) {
            printf("%s[  SKIPPED ]%s %s\n", colours[YELLOW], colours[RESET],
                   ubench_state.benchmarks[index].name);
            ubench_state.result = UBENCH_RESULT_PASS;
            continue;
        }

        if (UBENCH_RESULT_FAIL == ubench_state.result) {
            printf("%s[  FAILED  ]%s %s\n", colours[RED], colours[RESET],
                   ubench_state.benchmarks[index].name);
            {
                const size_t failed_benchmark_index = failed_benchmarks_length++;
                failed_benchmarks = (size_t *)realloc(
                    (void *)failed_benchmarks, sizeof(size_t) * failed_benchmarks_length);
                failed_benchmarks[failed_benchmark_index] = index;
                failed++;
            }
            ubench_state.result = UBENCH_RESULT_PASS;
            continue;
        }

        iterations = (100 * 1000 * 1000) / ((ns[1] <= ns[0]) ? 1 : ns[1] - ns[0]);
        iterations = iterations < min_iterations ? min_iterations : iterations;
        iterations = iterations > max_iterations ? max_iterations : iterations;

        for (mndex = 0; (mndex < 100) && (result != 0); mndex++) {
            dbj_ubt_int64_t kndex = 0;
            dbj_ubt_int64_t avg_ns = 0;
            double deviation = 0;
            double confidence = 0;

            iterations = iterations * ((dbj_ubt_int64_t)mndex + 1);
            iterations = iterations > max_iterations ? max_iterations : iterations;

            ubs.sample = 0;
            ubs.size = iterations;
            ubench_state.benchmarks[index].func(&ubs);

            for (kndex = 0; kndex < iterations; kndex++) {
                ns[kndex] = ns[kndex + 1] - ns[kndex];
            }
            for (kndex = 0; kndex < iterations; kndex++) {
                avg_ns += ns[kndex];
            }
            avg_ns /= iterations;

            for (kndex = 0; kndex < iterations; kndex++) {
                const double v = (double)(ns[kndex] - avg_ns);
                deviation += v * v;
            }
            deviation = sqrt(deviation / (double)iterations);

            confidence = 2.576 * deviation / sqrt((double)iterations);
            confidence = (confidence / (double)avg_ns) * 100.0;
            deviation = (deviation / (double)avg_ns) * 100.0;

            result = confidence > ubench_state.confidence;

            if (confidence < best_confidence) {
                best_avg_ns = avg_ns;
                best_deviation = deviation;
                best_confidence = confidence;
            }
        }

        if (result) {
            printf("confidence interval %f%% exceeds maximum permitted %f%%\n",
                   best_confidence, ubench_state.confidence);
        }

        if (ubench_state.output) {
            fprintf(ubench_state.output, "%s, %" DBJ_UBT_PRId64 ", %f, %f,\n",
                    ubench_state.benchmarks[index].name, best_avg_ns, best_deviation,
                    best_confidence);
        }

        {
            const char *const colour = (0 != result) ? colours[RED] : colours[GREEN];
            const char *const status = (0 != result) ? "[  FAILED  ]" : "[       OK ]";
            const char *unit = "us";

            if (0 != result) {
                const size_t failed_benchmark_index = failed_benchmarks_length++;
                failed_benchmarks = (size_t *)realloc(
                    (void *)failed_benchmarks, sizeof(size_t) * failed_benchmarks_length);
                failed_benchmarks[failed_benchmark_index] = index;
                failed++;
            }

            printf("%s%s%s %s (mean ", colour, status, colours[RESET],
                   ubench_state.benchmarks[index].name);

            for (mndex = 0; mndex < 2; mndex++) {
                if (best_avg_ns <= 1000000) break;
                best_avg_ns /= 1000;
                unit = (0 == mndex) ? "ms" : "s";
            }

            printf("%" DBJ_UBT_PRId64 ".%03" DBJ_UBT_PRId64 "%s, confidence interval +- %f%%)\n",
                   best_avg_ns / 1000, best_avg_ns % 1000, unit, best_confidence);
        }
    }

    printf("%s[==========]%s %" DBJ_UBT_PRIu64 " benchmarks ran.\n",
           colours[GREEN], colours[RESET], ran_benchmarks);
    printf("%s[  PASSED  ]%s %" DBJ_UBT_PRIu64 " benchmarks.\n", colours[GREEN],
           colours[RESET], ran_benchmarks - failed);

    if (0 != failed) {
        printf("%s[  FAILED  ]%s %" DBJ_UBT_PRIu64 " benchmarks, listed below:\n",
               colours[RED], colours[RESET], failed);
        for (index = 0; index < failed_benchmarks_length; index++) {
            printf("%s[  FAILED  ]%s %s\n", colours[RED], colours[RESET],
                   ubench_state.benchmarks[failed_benchmarks[index]].name);
        }
    }

cleanup:
    for (index = 0; index < ubench_state.benchmarks_length; index++) {
        free((void *)ubench_state.benchmarks[index].name);
    }
    free((void *)failed_benchmarks);
    free((void *)ubench_state.benchmarks);
    if (ubench_state.output) {
        fclose(ubench_state.output);
    }
    return (int)failed;
}

/* keeps the compiler from optimizing away work a benchmark produced but
   never otherwise uses -- pass the address of the value to preserve */
#define UBENCH_DO_NOTHING(x) ubench_do_nothing(x)
void ubench_do_nothing(void *ptr);

#define UBENCH_STATE()                                                        \
    void ubench_do_nothing(void *ptr) {                                       \
        __asm__ volatile("" : : "r"(ptr), "m"(ptr) : "memory");               \
    }                                                                        \
    struct ubench_state_s ubench_state = {0, 0, 0, 2.5, UBENCH_RESULT_PASS}

#endif /* DBJ_UBENCHTEST_H_INCLUDED */
