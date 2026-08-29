/*
    2026AUG29       (c) dbj@dbj.org

    Benchmarks for Troy D. Hanson's uthash, using toplevel/dbj_nanobench.h.

    uthash.h 2.4.0 is vendored in src/, unmodified, and reached by the
    -isystem in the Makefile -- third-party, so its own warnings stay
    out of this repo's -Werror.

    Same 256 keys, "key0".."key255", as the dbj_hashmap and Wellons
    benchmarks, so the three sit in one table.

    What is *not* comparable, and why it is measured apart:

    uthash is a chained hash that mallocs one entry per key and grows
    its bucket array as it fills. dbj_hashmap is flat, fixed-capacity,
    open-addressed, and allocates nothing.

    Comparing lookups is fair -- both answer the same question.
    Comparing inserts is not, even with the allocation taken out:
    uthash grows and rehashes where dbj_hashmap cannot, and a table
    that can outgrow its capacity is doing something the fixed one
    refuses to do. The insert number below is here for scale, not for
    the table.
*/
#include <dbj_required_compile_time.h>

#define DBJ_NANOBENCH_IMPLEMENTATION
#include <dbj_nanobench.h>

#include <dbj_clintro.h>

#include <uthash.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Name and version for dbj_clintro's banner. Hardcoded on purpose --
   see chris_welons/readme.md. */
#define DBJ_APP_NAME "uthash_benchmark"
#define DBJ_APP_VERSION "0.1.0"

#define BENCH_KEYS 256
#define BENCH_KEY_SIZE 16
#define BENCH_VALUE_SIZE 32

/* uthash's shape: the key is a member of the caller's own struct, and
   UT_hash_handle is the intrusive bookkeeping. Nothing about this is
   optional -- it is how the library works. */
typedef struct
{
    char key[BENCH_KEY_SIZE];
    char value[BENCH_VALUE_SIZE];
    UT_hash_handle hh;
} bench_entry;

static bench_entry *table = nullptr;

/* Filled once, measured many times -- as in the other two benchmarks,
   filling is not what is being timed. */
static void fill_table(void)
{
    for (int i = 0; i < BENCH_KEYS; i++)
    {
        bench_entry *entry = calloc(1, sizeof *entry);
        if (!entry)
        {
            return;
        }
        snprintf(entry->key, sizeof entry->key, "key%d", i);
        snprintf(entry->value, sizeof entry->value, "value%d", i);
        HASH_ADD_STR(table, key, entry);
    }
}

/* uthash's macros contain commas, and DBJ_BENCH takes its block as a
   single macro argument -- the preprocessor counts those commas as
   argument separators and the expansion fails. Wrapping each call in a
   function hides them, and costs nothing at -O2: these inline. */
[[nodiscard]] static inline bench_entry *find_key(const char *key)
{
    bench_entry *found = nullptr;
    HASH_FIND_STR(table, key, found);
    return found;
}

[[nodiscard]] static inline unsigned hash_key(const char *key, unsigned length)
{
    unsigned hashv = 0;
    HASH_VALUE(key, length, hashv);
    return hashv;
}

/* Insert, measured properly.

   DBJ_MEASURE times its block every iteration, so any setup or
   teardown written inside the block is timed with it. An insert
   benchmark cannot use it: allocating the entry, and clearing the
   table so the next iteration starts empty, would both land in the
   number.

   So this times the clock directly. One run fills a table with all
   BENCH_KEYS entries -- allocated beforehand, freed afterwards, both
   outside the timed region -- and reports the per-key cost. Filling
   the whole table is the point: uthash grows and rehashes its bucket
   array as it goes, and inserting into a permanently empty table
   would measure the one case where that never happens. */
[[nodiscard]] static double fill_ns_per_key(bench_entry *entries[static BENCH_KEYS])
{
    bench_entry *fresh = nullptr;

    uint64_t started = DBJ_NB_now_ns();
    for (int i = 0; i < BENCH_KEYS; i++)
    {
        HASH_ADD_STR(fresh, key, entries[i]);
    }
    uint64_t elapsed = DBJ_NB_now_ns() - started;

    /* HASH_CLEAR unlinks; the entries themselves are the caller's and
       are reused for the next run. */
    HASH_CLEAR(hh, fresh);
    return (double)elapsed / BENCH_KEYS;
}

/* Reports like DBJ_MEASURE does, so the line sits with the others. */
static void bench_insert(void)
{
    bench_entry *entries[BENCH_KEYS] = {};
    for (int i = 0; i < BENCH_KEYS; i++)
    {
        entries[i] = calloc(1, sizeof *entries[i]);
        if (!entries[i])
        {
            return;
        }
        snprintf(entries[i]->key, sizeof entries[i]->key, "key%d", i);
    }

    DBJ_NB_result result = {.name = "uthash insert, per key (grows)",
                            .warmup_iters = 100,
                            .iters = 1000,
                            .min_ns = UINT64_MAX};

    for (uint64_t i = 0; i < result.warmup_iters; i++)
    {
        (void)fill_ns_per_key(entries);
    }
    for (uint64_t i = 0; i < result.iters; i++)
    {
        uint64_t per_key = (uint64_t)fill_ns_per_key(entries);
        result.total_ns += per_key;
        if (per_key < result.min_ns)
        {
            result.min_ns = per_key;
        }
        if (per_key > result.max_ns)
        {
            result.max_ns = per_key;
        }
    }
    DBJ_NB_report(&result);

    for (int i = 0; i < BENCH_KEYS; i++)
    {
        free(entries[i]);
    }
}

int main(int argc, char *argv[static argc + 1])
{
    (void)argv;
    dbj_clintro(DBJ_APP_NAME, DBJ_APP_VERSION);

    fill_table();
    printf("%u keys in the table\n\n", HASH_COUNT(table));

    DBJ_BENCH("uthash find, key present", bench_entry *, {
        DBJ_NB_val = find_key("key100");
    });

    DBJ_BENCH("uthash find, key absent", bench_entry *, {
        DBJ_NB_val = find_key("nosuchkey");
    });

    /* uthash's own hash of the same key, for scale against the other
       two benchmarks' hash lines. HASH_FUNCTION is the library's
       default (Jenkins one-at-a-time) unless overridden. */
    DBJ_BENCH("uthash hash of a short key", unsigned, {
        DBJ_NB_val = hash_key("key100", 6);
    });

    /* Apart from the table above -- see the note at the top. The
       entries are allocated outside the timed region, so this is
       uthash's insert and not the allocator's. */
    bench_insert();

    HASH_CLEAR(hh, table);
    return EXIT_SUCCESS;
}
