/*
    2026AUG29       (c) dbj@dbj.org

    Benchmarks for Chris Wellons' core lib, using toplevel/dbj_nanobench.h.

    His code is not copied here. WELLONS_AS_LIB suppresses the main()
    in wellons-corelib.c, and the whole file is #included --
    one #define turning a program into a library. On the edge of a
    hack, and exactly the sort of thing C permits.
*/
#include <dbj_required_compile_time.h>

#define DBJ_NANOBENCH_IMPLEMENTATION
#include <dbj_nanobench.h>

#include <dbj_clintro.h>

#define WELLONS_AS_LIB
#include "wellons-corelib.c"

#include <stdlib.h>

/* Name and version for dbj_clintro's banner. Hardcoded on purpose --
   see chris_welons/readme.md. */
#define DBJ_APP_NAME "wellons_benchmark"
#define DBJ_APP_VERSION "0.5.0"

#define BENCH_KEYS 256
#define BENCH_ARENA_SIZE (1 << 24)

int main(int argc, char *argv[static argc + 1])
{
    (void)argv;
    dbj_clintro(DBJ_APP_NAME, DBJ_APP_VERSION);

    char *block = malloc(BENCH_ARENA_SIZE);
    if (!block)
    {
        return EXIT_FAILURE;
    }
    Arena arena = {block, block + BENCH_ARENA_SIZE};

    /* Two tables filled from one arena, then measured. Filling is not
       measured: it would time the arena as much as the table. */
    FlatEnv *flat = new (&arena, 1, FlatEnv);
    Env *trie = 0;
    for (int i = 0; i < BENCH_KEYS; i++)
    {
        Str key = print(&arena, "key%d", i);
        Str value = print(&arena, "value%d", i);
        *flatlookup(flat, key) = value;
        *lookup(&trie, key, &arena) = value;
    }

    DBJ_BENCH("flatlookup, key present", Str *, {
        DBJ_NB_val = flatlookup(flat, S("key100"));
    });

    DBJ_BENCH("hashtrie lookup, key present", Str *, {
        DBJ_NB_val = lookup(&trie, S("key100"), 0);
    });

    DBJ_BENCH("hashtrie lookup, key absent", Str *, {
        DBJ_NB_val = lookup(&trie, S("nosuchkey"), 0);
    });

    DBJ_BENCH("hash64 of a short key", uint64_t, {
        DBJ_NB_val = hash64(S("key100"));
    });

    /* append and print both allocate, so each iteration is given its
       own scratch copy of an arena that is never rewound -- the cost
       shown includes the bump, which is how they are actually used. */
    DBJ_BENCH("append onto a slice", StrSlice, {
        Arena scratch = arena;
        StrSlice words = {0};
        DBJ_NB_val = append(&scratch, words, S("word"));
    });

    DBJ_BENCH("print into the arena", Str, {
        Arena scratch = arena;
        DBJ_NB_val = print(&scratch, "value%d", 100);
    });

    free(block);
    return EXIT_SUCCESS;
}
