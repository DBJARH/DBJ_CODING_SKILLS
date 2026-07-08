/*
    Try-out for ubenchtest.h -- confirms, by actually compiling and
    running:

    1. UBENCH() is the only unit of work -- no UTEST(), no test
       registration/filtering/sorting machinery.
    2. A UBENCH() body can use the non-fatal EXPECT_* assertion macros
       (EXPECT_EQ, EXPECT_TRUE, ...) after declaring ENABLE_UBNCH_ASSERT_MACROS()
       as its first statement. A failing EXPECT_* prints its own
       Expected/Actual failure line but does not fail or truncate the
       benchmark -- timing/sampling still runs to completion, and the
       benchmark still reports [ OK ]. The fatal ASSERT_* family is
       deliberately not used here -- see ubenchtest.h for why it does
       not play well with UBENCH()'s sample loop.
    3. ubench benchmarks register in constructor order, which on GCC 15
       / MinGW-w64 is the REVERSE of source declaration order (see
       ubenchtest.h) -- do not rely on it.

    gcc -std=c2x -Wall -Wswitch -Werror -o ubenchtest ubenchtest.c
*/
#include "ubenchtest.h"

UBENCH_STATE();

static int is_prime(const int x) {
    if (x < 2) return 0;
    for (int d = 2; (d * d) <= x; d++) {
        if (0 == (x % d)) return 0;
    }
    return 1;
}

/* 1-indexed: get_nth_prime(1) == 2, get_nth_prime(2) == 3, ... */
static int get_nth_prime(const int n) {
    int count = 0;
    int candidate = 1;
    while (count < n) {
        candidate++;
        if (is_prime(candidate)) {
            count++;
        }
    }
    return candidate;
}

/* only EXPECT_* is usable here in practice: ASSERT_* compiles but its
   early return on failure clashes with UBENCH_DO_BENCHMARK()'s sample
   loop, so a real failure reprints once per sample instead of once --
   see ubenchtest.h */
UBENCH(DBJ, prime_1000) {
    ENABLE_UBNCH_ASSERT_MACROS();

    volatile int result = get_nth_prime(1000);
    /* stops the compiler from optimizing result away as dead, since
       nothing above uses it yet -- see UBENCH_DO_NOTHING in ubenchtest.h */
    UBENCH_DO_NOTHING((void *)&result);

    EXPECT_EQ(result, 7919);
    EXPECT_NE(result, 0);
    EXPECT_GT(result, 0);
    EXPECT_GE(result, 7919);
    EXPECT_LT(result, 10000);
    EXPECT_LE(result, 7919);
    EXPECT_TRUE(result > 1);
    EXPECT_FALSE(result < 0);
    /* stops the outer sample loop after the first failure -- see
       DBJ.sum_first_n_and_check below for why this call matters */
    UBENCH_ASSERT_SCOPE_CHECK();
}

UBENCH(DBJ, prime_100) {
    ENABLE_UBNCH_ASSERT_MACROS();

    volatile int result = get_nth_prime(100);
    UBENCH_DO_NOTHING((void *)&result);

    EXPECT_EQ(result, 541);
    EXPECT_NE(result, 0);
    EXPECT_GT(result, 0);
    EXPECT_GE(result, 541);
    EXPECT_LT(result, 1000);
    EXPECT_LE(result, 541);
    EXPECT_TRUE(result > 1);
    EXPECT_FALSE(result < 0);
    UBENCH_ASSERT_SCOPE_CHECK();
}

UBENCH(DBJ, prime_10) {
    ENABLE_UBNCH_ASSERT_MACROS();

    volatile int result = get_nth_prime(10);
    UBENCH_DO_NOTHING((void *)&result);

    EXPECT_EQ(result, 29);
    EXPECT_NE(result, 0);
    EXPECT_GT(result, 0);
    EXPECT_GE(result, 29);
    EXPECT_LT(result, 100);
    EXPECT_LE(result, 29);
    EXPECT_TRUE(result > 1);
    EXPECT_FALSE(result < 0);
    UBENCH_ASSERT_SCOPE_CHECK();
}

/* a benchmark that also sanity-checks its own result via assertion
   macros -- a failing EXPECT_* prints a failure line but does not fail
   this benchmark or abort the run. */
UBENCH(DBJ, sum_first_n_and_check) {
    ENABLE_UBNCH_ASSERT_MACROS();
    EXPECT_EQ(true, false);
    UBENCH_ASSERT_SCOPE_CHECK();
}

int main(const int argc, const char *const argv[static argc + 1]) {
    return ubench_main(argc, argv);
}
