
#ifndef DBJ_MACROS_H
#define DBJ_MACROS_H
/**
 DBJ_LOOP(n) { ... loop_counter ... }
 DBJ_LOOP_AS(name, n) { ... name ... }

 Counts up from 0 to n-1 (exclusive), same range as
 `for (size_t loop_counter = 0; loop_counter < n; loop_counter++)`,
 but reads as a keyword instead of loop boilerplate.

 DBJ_LOOP always names its counter `loop_counter` -- fine for a single,
 unnested loop, but nesting two of them shadows the outer counter with
 no way to reach it from the inner body. DBJ_LOOP_AS takes the counter
 name as an argument instead, so nested loops (e.g. i/j/k over a matrix)
 stay simultaneously reachable:

     DBJ_LOOP_AS(i, n) {
         DBJ_LOOP_AS(j, n) {
             double sum = 0;
             DBJ_LOOP_AS(k, n) { sum += A[i][k] * B[k][j]; }
             C[i][j] = sum;
         }
     }

 Unique names per loop so sibling DBJ_LOOP/DBJ_LOOP_AS uses in the same
 block do not collide on their hidden end-bound variable, same
 __COUNTER__ trick as toplevel/dbj_defer.h.
*/
#include "dbj_required_compile_time.h"

#include <stddef.h>

#define DBJ_LOOP(n_) DBJ_LOOP_AS(loop_counter, n_)

#define DBJ_LOOP_AS(name_, n_) __DBJ_LOOP_AS(__COUNTER__, name_, (n_))
#define __DBJ_LOOP_AS(id_, name_, n_) ___DBJ_LOOP_AS(id_, name_, (n_))
#define ___DBJ_LOOP_AS(id_, name_, n_)                                \
    for (size_t __DBJ_LOOP_END_##id_ = (size_t)(n_),                  \
                name_ = 0;                                            \
         name_ < __DBJ_LOOP_END_##id_;                                \
         name_++)

#endif // DBJ_MACROS_H
