/*
tau_bench.h -- micro-benchmark timer macros layered on top of Tau's
own internal clock (tauClock() / tauClockPrintDuration() in tau.h).

This is NOT part of upstream Tau. It is kept as a separate header,
with a DBJ_TAU_ prefix on everything it adds, so that re-vendoring
tau.h from upstream never clobbers it and so it's obvious at the call
site which macros are ours vs upstream Tau's.

Usage (inside a TEST body):

    #include <tau/tau.h>
    #include <tau/tau_bench.h>
    TAU_MAIN()

    TEST(suite, name) {
        DBJ_TAU_START_TIMER(work);
        do_work();
        DBJ_TAU_PRINT_ELAPSED(work);        // prints e.g. "1.23ms"
        CHECK_LT(DBJ_TAU_ELAPSED(work), 5.0 * 1000 * 1000); // under 5ms, in ns
    }

Requires TAU_NO_TESTING to be undefined, since tauClock() only exists
in Tau's normal testing mode.
*/

#ifndef DBJ_TAU_BENCH_H_
#define DBJ_TAU_BENCH_H_

#include <tau/tau.h>

#ifdef TAU_NO_TESTING
    #error "tau_bench.h requires Tau's normal testing mode (TAU_NO_TESTING must not be defined)"
#endif

// Starts a named timer. Declares a local variable scoped to the
// enclosing block, so each name must be unique within that block.
// CAUTION: `name` is token-pasted into a real C identifier, so this
// is not hygienic like a real benchmark framework's timer object --
// reusing the same name in a nested/overlapping scope shadows the
// outer timer (expect a -Wshadow warning, not a compile error).
#define DBJ_TAU_START_TIMER(name) \
    double name##_dbj_tau_bench_start_ = tauClock()

// Nanoseconds elapsed since the matching DBJ_TAU_START_TIMER(name).
// May be called more than once against the same timer.
#define DBJ_TAU_ELAPSED(name) \
    (tauClock() - (name##_dbj_tau_bench_start_))

// Prints the elapsed time for `name`, auto-scaled (ns/us/ms/s) via
// Tau's own tauClockPrintDuration(), followed by a newline.
#define DBJ_TAU_PRINT_ELAPSED(name)                    \
    do {                                                \
        tauClockPrintDuration(DBJ_TAU_ELAPSED(name));   \
        tauPrintf("\n");                                \
    } while(0)

#endif // DBJ_TAU_BENCH_H_
