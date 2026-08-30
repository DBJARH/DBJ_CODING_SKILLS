/*
    2026AUG29       (c) dbj@dbj.org

    Benchmarks for Chris Wellons' core lib, using
    third_party/dbj_ubenchtest.

    His code is not copied here. WELLONS_AS_LIB suppresses the main()
    in wellons-corelib.c, and the whole file is #included --
    one #define turning a program into a library. On the edge of a
    hack, and exactly the sort of thing C permits.

    The two tables are filled once, before any benchmark runs. Filling
    is not measured: it would time the arena as much as the table.

    Benchmarks come out in the linker's order, not this file's.
*/
#include <dbj_required_compile_time.h>

#include <dbj_ubenchtest.h>

#include <dbj_clintro.h>

#define WELLONS_AS_LIB
#include "wellons-corelib.c"

#include <stdlib.h>

UBENCH_STATE();

/* Name and version for dbj_clintro's banner. Hardcoded on purpose --
   see chris_welons/readme.md. */
#define DBJ_APP_NAME "wellons_benchmark"
#define DBJ_APP_VERSION "0.6.0"

#define BENCH_KEYS 256
#define BENCH_ARENA_SIZE (1 << 24)

/* Filled by main() before ubench_main() hands over. File scope because
   a benchmark body is a function of its own. */
static Arena bench_arena = {0};
static FlatEnv *flat = 0;
static Env *trie = 0;

UBENCH(wellons, flatlookup_present)
{
    Str *found = flatlookup(flat, S("key100"));
    UBENCH_DO_NOTHING((void *)&found);
}

UBENCH(wellons, hashtrie_lookup_present)
{
    Str *found = lookup(&trie, S("key100"), 0);
    UBENCH_DO_NOTHING((void *)&found);
}

UBENCH(wellons, hashtrie_lookup_absent)
{
    Str *found = lookup(&trie, S("nosuchkey"), 0);
    UBENCH_DO_NOTHING((void *)&found);
}

UBENCH(wellons, hash64_short_key)
{
    uint64_t hashv = hash64(S("key100"));
    UBENCH_DO_NOTHING((void *)&hashv);
}

/* append and print both allocate, so each call is given its own
   scratch copy of an arena that is never rewound -- the cost shown
   includes the bump, which is how they are actually used. */
UBENCH(wellons, append_onto_slice)
{
    Arena scratch = bench_arena;
    StrSlice words = {0};
    StrSlice grown = append(&scratch, words, S("word"));
    UBENCH_DO_NOTHING((void *)&grown);
}

UBENCH(wellons, print_into_arena)
{
    Arena scratch = bench_arena;
    Str printed = print(&scratch, "value%d", 100);
    UBENCH_DO_NOTHING((void *)&printed);
}

int main(const int argc, const char *const argv[static argc + 1])
{
    dbj_clintro(DBJ_APP_NAME, DBJ_APP_VERSION);

    char *block = malloc(BENCH_ARENA_SIZE);
    if (!block)
    {
        return EXIT_FAILURE;
    }
    bench_arena = (Arena){block, block + BENCH_ARENA_SIZE};

    flat = new (&bench_arena, 1, FlatEnv);
    for (int i = 0; i < BENCH_KEYS; i++)
    {
        Str key = print(&bench_arena, "key%d", i);
        Str value = print(&bench_arena, "value%d", i);
        *flatlookup(flat, key) = value;
        *lookup(&trie, key, &bench_arena) = value;
    }

    int failed = ubench_main(argc, argv);

    free(block);
    return failed;
}
