/*
    2026AUG29       (c) dbj@dbj.org

    Benchmarks for dbj_hashmap.h, using toplevel/dbj_nanobench.h.

        get, key present    the whole read path: probe, then copy out
        get, key absent     a miss probes until it meets an empty slot
        probe only          the search alone, no element copy
        hash_string_32 fill building a value, for scale

    Then the same reads with a string key and again with a slice key,
    which is what makes this comparable to
    chris_welons/wellons_benchmark.c -- same key count, same
    "key0".."key255" data, same questions.

    The ordinal numbers above are the floor: a key that is its own hash
    cannot be beaten by one that must be hashed and compared. Between
    the two text kinds, the slice is the one to watch -- it hashes the
    bytes that are there, where the owned string hashes its whole fixed
    buffer whatever the key's actual length.
*/
#include <dbj_required_compile_time.h>

/* dbj_result.h's factories belong to one translation unit; this is it. */
#define DBJ_MAKERESULT_IMPLEMENTATION
#include "dbj_hashmap.h"

#define DBJ_NANOBENCH_IMPLEMENTATION
#include <dbj_nanobench.h>

#include <dbj_clintro.h>

#include <stdio.h>

/* Name and version for dbj_clintro's banner, and for any message this
   app prints about itself.

   Hardcoded on purpose. There is no build-time version generation in
   this repo and none is wanted here -- dbj_the_game/build_timestamp.inc
   does that, and it is overkill for a benchmark. The record that counts
   is the git tag and the `version:` front matter in readme.md; this
   string is a courtesy to whoever is reading the terminal, so bump it
   by hand when the file changes meaningfully. */
#define DBJ_APP_NAME "dbj_hashmap_benchmarks"
#define DBJ_APP_VERSION "0.8.0"

#define DBJ_BENCH_KEYS 256
#define DBJ_BENCH_PROBE 100
#define DBJ_BENCH_ABSENT 9999
#define DBJ_BENCH_ARENA (1 << 16)

/* Filled once, measured many times. File scope, not arena-allocated:
   the arena is not what is being measured. */
static dbj_hashmap bench_map;
static dbj_hashmap strkey_map;
static dbj_hashmap slice_map;

/* The block the slice keys' text lives in. File scope so it outlives
   the map, which is the lifetime the slice kind requires; the arena
   below only points into it. */
static char slice_arena_block[DBJ_BENCH_ARENA];

int main(int argc, char *argv[static argc + 1])
{
    (void)argv;
    dbj_clintro(DBJ_APP_NAME, DBJ_APP_VERSION);

    for (KeyOrdinal key = 0; key < DBJ_BENCH_KEYS; key++)
    {
        (void)dbj_hashmap_set(&bench_map, hash_key_ordinal(key), hash_string_32("v"));
    }
    printf("%td of %d slots filled\n\n",
           dbj_hashmap_count(&bench_map), DBJ_HASHMAP_SLOTS);

    DBJ_BENCH("hashmap get, key present", HashMapElementResult, {
        DBJ_NB_val = dbj_hashmap_get(&bench_map, hash_key_ordinal(DBJ_BENCH_PROBE));
    });

    DBJ_BENCH("hashmap get, key absent", HashMapElementResult, {
        DBJ_NB_val = dbj_hashmap_get(&bench_map, hash_key_ordinal(DBJ_BENCH_ABSENT));
    });

    DBJ_BENCH("hashmap probe only", dbj_hashmap_probe, {
        DBJ_NB_val = dbj_hashmap_slot(&bench_map, hash_key_ordinal(DBJ_BENCH_PROBE));
    });

    DBJ_BENCH("hash_string_32 fill", HashString, {
        DBJ_NB_val = hash_string_32("value100");
    });

    /* String keys, filled the way the Wellons benchmark fills its
       tables -- "key0".."key255" -- so the two runs are asking the
       same question of the same data. */
    for (int i = 0; i < DBJ_BENCH_KEYS; i++)
    {
        char key[32];
        char value[32];
        snprintf(key, sizeof(key), "key%d", i);
        snprintf(value, sizeof(value), "value%d", i);
        (void)dbj_hashmap_set(&strkey_map, hash_key_string(key), hash_string_32(value));
    }

    DBJ_BENCH("string key get, key present", HashMapElementResult, {
        DBJ_NB_val = dbj_hashmap_get(&strkey_map, hash_key_string("key100"));
    });

    DBJ_BENCH("string key get, key absent", HashMapElementResult, {
        DBJ_NB_val = dbj_hashmap_get(&strkey_map, hash_key_string("nosuchkey"));
    });

    DBJ_BENCH("string key hash alone", uint64_t, {
        DBJ_NB_val = hash_key_hash(hash_key_string("key100"));
    });

    /* Slice keys, same data again, the text held in an arena. A slice
       hashes only the bytes that are there -- six for "key100" --
       where KT_STRING hashes all 32 of its fixed buffer. That
       difference is the whole point of having both kinds. */
    dbj_arena arena = dbj_arena_make(slice_arena_block, DBJ_BENCH_ARENA);

    for (int i = 0; i < DBJ_BENCH_KEYS; i++)
    {
        char text[32];
        snprintf(text, sizeof(text), "key%d", i);

        char value[32];
        snprintf(value, sizeof(value), "value%d", i);

        /* the text is copied into the arena, which outlives the map --
           `text` above is a stack buffer and would not do */
        (void)dbj_hashmap_set(&slice_map, hash_key_slice_copy(&arena, text),
                              hash_string_32(value));
    }

    DBJ_BENCH("slice key get, key present", HashMapElementResult, {
        DBJ_NB_val = dbj_hashmap_get(&slice_map, hash_key_slice(DBJ_SS("key100")));
    });

    DBJ_BENCH("slice key get, key absent", HashMapElementResult, {
        DBJ_NB_val = dbj_hashmap_get(&slice_map, hash_key_slice(DBJ_SS("nosuchkey")));
    });

    DBJ_BENCH("slice key hash alone", uint64_t, {
        DBJ_NB_val = hash_key_hash(hash_key_slice(DBJ_SS("key100")));
    });

    return 0;
}
