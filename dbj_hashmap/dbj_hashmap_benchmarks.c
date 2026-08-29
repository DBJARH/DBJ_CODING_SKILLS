/*
    2026AUG29       (c) dbj@dbj.org

    Benchmarks for dbj_hashmap.h, using toplevel/dbj_nanobench.h.

        get, key present    the whole read path: probe, then copy out
        get, key absent     a miss probes until it meets an empty slot
        probe only          the search alone, no element copy
        hash_string_32 fill building a value, for scale

    Not a comparison against anything -- these are here so a change to
    the probe or to HashString can be seen rather than argued about.
*/
#include <dbj_required_compile_time.h>

/* dbj_result.h's factories belong to one translation unit; this is it. */
#define DBJ_MAKERESULT_IMPLEMENTATION
#include "dbj_hashmap.h"

#define DBJ_NANOBENCH_IMPLEMENTATION
#include <dbj_nanobench.h>

#include <stdio.h>

#define DBJ_BENCH_KEYS 256
#define DBJ_BENCH_PROBE 100
#define DBJ_BENCH_ABSENT 9999

/* Filled once, measured many times. File scope, not arena-allocated:
   the arena is not what is being measured. */
static dbj_hashmap bench_map;

int main(void)
{
    for (KeyType key = 0; key < DBJ_BENCH_KEYS; key++)
    {
        (void)dbj_hashmap_set(&bench_map, key, hash_string_32("v"));
    }
    printf("dbj_hashmap benchmarks -- %td of %d slots filled\n\n",
           dbj_hashmap_count(&bench_map), DBJ_HASHMAP_SLOTS);

    DBJ_BENCH("hashmap get, key present", HashMapElementResult, {
        DBJ_NB_val = dbj_hashmap_get(&bench_map, DBJ_BENCH_PROBE);
    });

    DBJ_BENCH("hashmap get, key absent", HashMapElementResult, {
        DBJ_NB_val = dbj_hashmap_get(&bench_map, DBJ_BENCH_ABSENT);
    });

    DBJ_BENCH("hashmap probe only", dbj_hashmap_probe, {
        DBJ_NB_val = dbj_hashmap_slot(&bench_map, DBJ_BENCH_PROBE);
    });

    DBJ_BENCH("hash_string_32 fill", HashString, {
        DBJ_NB_val = hash_string_32("value100");
    });

    return 0;
}
