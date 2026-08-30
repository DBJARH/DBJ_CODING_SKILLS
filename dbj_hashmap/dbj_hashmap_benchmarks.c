/*
    2026AUG31       (c) dbj@dbj.org

    Benchmarks for dbj_hashmap.h, using third_party/dbj_ubenchtest.

        get, key present    the whole read path: probe, then copy out
        get, key absent     a miss probes until it meets an empty slot
        probe only          the search alone, no element copy
        hash_string_32 fill building a value, for scale

    Then the same reads with a string key and again with a slice key,
    which is what makes this comparable to
    chris_welons/wellons_benchmark.c -- same key count, same
    "key0".."key255" data, same questions.

    The ordinal numbers are the floor: a key that is its own hash cannot
    be beaten by one that must be hashed and compared. Between the two
    text kinds, the slice is the one to watch -- it hashes the bytes that
    are there, where the owned string hashes its whole fixed buffer
    whatever the key's actual length.

    Benchmarks come out in the linker's order, not this file's. Each set
    name (ordinal, string, slice) says which key kind the line is about,
    so a shuffled list still reads.

    Last, an insert measured the way dbj_uthash measures its own, so
    the two are read side by side -- with the asymmetry stated at that
    function rather than buried in a table.
*/
#include <dbj_required_compile_time.h>

/* dbj_result.h's factories belong to one translation unit; this is it. */
#define DBJ_MAKERESULT_IMPLEMENTATION
#include "dbj_hashmap.h"

#include <dbj_ubenchtest.h>
#include <dbj_clintro.h>

#include <stdio.h>

UBENCH_STATE();

/* The generated names carry both type arguments. Shorthands here in the
   benchmark only -- the map itself has no alias layer. */
typedef hashmap_HashKey_HashString map_t;
typedef hashmap_HashKey_HashString_elementResult map_result;

#define hashmap_get hashmap_HashKey_HashString_get
#define hashmap_set hashmap_HashKey_HashString_set
#define hashmap_count hashmap_HashKey_HashString_count
#define hashmap_slot hashmap_HashKey_HashString_slot

/* Name and version for dbj_clintro's banner, and for any message this
   app prints about itself.

   Hardcoded on purpose. There is no build-time version generation in
   this repo and none is wanted here -- dbj_the_game/build_timestamp.inc
   does that, and it is overkill for a benchmark. The record that counts
   is the git tag and the `version:` front matter in readme.md; this
   string is a courtesy to whoever is reading the terminal, so bump it
   by hand when the file changes meaningfully. */
#define DBJ_APP_NAME "dbj_hashmap_benchmarks"
#define DBJ_APP_VERSION "1.2.0"

#define DBJ_BENCH_KEYS 256
#define DBJ_BENCH_PROBE 100
#define DBJ_BENCH_ABSENT 9999
#define DBJ_BENCH_ARENA 65536

/* Filled once, measured many times. File scope, not arena-allocated:
   the arena is not what is being measured. */
static map_t bench_map;
static map_t strkey_map;
static map_t slice_map;

/* The insert benchmark empties its map on every run, so it gets one of
   its own -- bench_map is filled once and read by the lookups above. */
static map_t insert_map;

/* The block the slice keys' text lives in. File scope so it outlives
   the map, which is the lifetime the slice kind requires; the arena
   below only points into it. */
static char slice_arena_block[DBJ_BENCH_ARENA];

/* ------------------------------------------------------------------
   the benchmarks

   One operation per body. These are all quicker than the clock can
   see one call at a time; ubenchtest handles that itself by running
   the body many times between two clock readings and dividing back
   out. It prints how many, per line.
   ------------------------------------------------------------------ */

UBENCH(ordinal, get_present)
{
    map_result result = hashmap_get(&bench_map, hash_key_ordinal(DBJ_BENCH_PROBE));
    UBENCH_DO_NOTHING((void *)&result);
}

UBENCH(ordinal, get_absent)
{
    map_result result = hashmap_get(&bench_map, hash_key_ordinal(DBJ_BENCH_ABSENT));
    UBENCH_DO_NOTHING((void *)&result);
}

UBENCH(ordinal, probe_only)
{
    dbj_hashmap_probe probe = hashmap_slot(&bench_map, hash_key_ordinal(DBJ_BENCH_PROBE));
    UBENCH_DO_NOTHING((void *)&probe);
}

/* not a map operation at all -- the cost of building a value, for scale */
UBENCH(value, hash_string_32_fill)
{
    HashString value = hash_string_32("value100");
    UBENCH_DO_NOTHING((void *)&value);
}

UBENCH(string, get_present)
{
    map_result result = hashmap_get(&strkey_map, hash_key_string("key100"));
    UBENCH_DO_NOTHING((void *)&result);
}

UBENCH(string, get_absent)
{
    map_result result = hashmap_get(&strkey_map, hash_key_string("nosuchkey"));
    UBENCH_DO_NOTHING((void *)&result);
}

UBENCH(string, hash_alone)
{
    uint64_t hash = HashKey_hash(hash_key_string("key100"));
    UBENCH_DO_NOTHING((void *)&hash);
}

UBENCH(slice, get_present)
{
    map_result result = hashmap_get(&slice_map, hash_key_slice(DBJ_SS("key100")));
    UBENCH_DO_NOTHING((void *)&result);
}

UBENCH(slice, get_absent)
{
    map_result result = hashmap_get(&slice_map, hash_key_slice(DBJ_SS("nosuchkey")));
    UBENCH_DO_NOTHING((void *)&result);
}

UBENCH(slice, hash_alone)
{
    uint64_t hash = HashKey_hash(hash_key_slice(DBJ_SS("key100")));
    UBENCH_DO_NOTHING((void *)&hash);
}

/* ------------------------------------------------------------------
   insert, timed by hand
   ------------------------------------------------------------------ */

/* Insert, measured the same way dbj_uthash/uthash_benchmark.c measures
   it: time a full DBJ_BENCH_KEYS fill and divide, so the number is
   per key and the two are read side by side.

   UBENCH() cannot do this. It times its whole body every sample, and an
   insert benchmark needs the map emptied between runs -- that clear
   would be timed with the inserts, and clearing this map moves far more
   memory than 256 inserts do. So this one keeps its own clock.

   What the two numbers do NOT say is which table is better at
   inserting. uthash grows and rehashes as it fills and would take a
   100,000th key; this map has 1024 slots and answers ERR past them.
   Refusing to grow is most of why it is quicker. Same load, different
   promises. */
[[nodiscard]] static double fill_ns_per_key(map_t *map, HashString value)
{
    dbj_hashmap_clear(map); /* a zeroed map is an empty map */

    int64_t started = dbj_ubt_ns();
    for (KeyOrdinal key = 0; key < DBJ_BENCH_KEYS; key++)
    {
        (void)hashmap_set(map, hash_key_ordinal(key), value);
    }
    int64_t elapsed = dbj_ubt_ns() - started;

    return (double)elapsed / DBJ_BENCH_KEYS;
}

static void bench_insert(map_t *map)
{
    HashString value = hash_string_32("v");
    double best = 1e30;
    double total = 0.0;

    for (int run = 0; run < 100; run++)
    {
        (void)fill_ns_per_key(map, value); /* warmup */
    }
    for (int run = 0; run < 1000; run++)
    {
        double per_key = fill_ns_per_key(map, value);
        total += per_key;
        if (per_key < best)
        {
            best = per_key;
        }
    }

    printf("ordinal key insert, per key: avg %.2f ns, min %.2f ns (n=1000)\n\n",
           total / 1000.0, best);
}

/* ------------------------------------------------------------------
   filling the maps -- untimed, before any benchmark runs
   ------------------------------------------------------------------ */

static int fill_the_maps(void)
{
    for (KeyOrdinal key = 0; key < DBJ_BENCH_KEYS; key++)
    {
        (void)hashmap_set(&bench_map, hash_key_ordinal(key), hash_string_32("v"));
    }

    /* String keys, filled the way the Wellons benchmark fills its
       tables -- "key0".."key255" -- so the two runs are asking the
       same question of the same data. */
    for (int i = 0; i < DBJ_BENCH_KEYS; i++)
    {
        char key[32];
        char value[32];
        snprintf(key, sizeof(key), "key%d", i);
        snprintf(value, sizeof(value), "value%d", i);
        (void)hashmap_set(&strkey_map, hash_key_string(key), hash_string_32(value));
    }

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
           `text` above is a stack buffer and would not do. The arena is
           sized for the whole run, so an ERR here means the benchmark
           is misconfigured, not that the map is slow. */
        HashKeyResult key = hash_key_slice_copy(&arena, text);
        if (dbj_result_is_err(key))
        {
            fprintf(stderr, "%s: %s\n", dbj_result_location(HashKey, key),
                    dbj_result_message(HashKey, key));
            return 1;
        }
        (void)hashmap_set(&slice_map, dbj_result_hash_key(key), hash_string_32(value));
    }

    return 0;
}

int main(const int argc, const char *const argv[static argc + 1])
{
    dbj_clintro(DBJ_APP_NAME, DBJ_APP_VERSION);

    if (fill_the_maps() != 0)
    {
        return 1;
    }
    printf("%td of %d slots filled\n\n", hashmap_count(&bench_map), DBJ_HASHMAP_SLOTS);

    bench_insert(&insert_map);

    return ubench_main(argc, argv);
}
