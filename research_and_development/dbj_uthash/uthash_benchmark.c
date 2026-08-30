/*
    2026AUG29       (c) dbj@dbj.org

    Benchmarks for Troy D. Hanson's uthash, using
    third_party/dbj_ubenchtest.

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

#include <dbj_ubenchtest.h>

#include <dbj_clintro.h>

#include <uthash.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

UBENCH_STATE();

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

/* uthash's macros contain commas, which used to break the old
   harness's block-as-one-macro-argument form. Kept as functions
   anyway: they cost nothing at -O2, and a benchmark body reads better
   as one call. */
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

   UBENCH times the whole body every sample, so any setup or teardown
   written inside it is timed with it. An insert benchmark cannot use
   it: allocating the entry, and clearing the table so the next
   iteration starts empty, would both land in the number.

   So this times the clock directly. One run fills a table with all
   BENCH_KEYS entries -- allocated beforehand, freed afterwards, both
   outside the timed region -- and reports the per-key cost. Filling
   the whole table is the point: uthash grows and rehashes its bucket
   array as it goes, and inserting into a permanently empty table
   would measure the one case where that never happens. */
[[nodiscard]] static double fill_ns_per_key(bench_entry *entries[static BENCH_KEYS])
{
    bench_entry *fresh = nullptr;

    int64_t started = dbj_ubt_ns();
    for (int i = 0; i < BENCH_KEYS; i++)
    {
        HASH_ADD_STR(fresh, key, entries[i]);
    }
    int64_t elapsed = dbj_ubt_ns() - started;

    /* HASH_CLEAR unlinks; the entries themselves are the caller's and
       are reused for the next run. */
    HASH_CLEAR(hh, fresh);
    return (double)elapsed / BENCH_KEYS;
}

/* Printed before the ubenchtest lines, in the same shape
   dbj_hashmap's insert benchmark uses, so the two read side by side. */
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

    double best = 1e30;
    double total = 0.0;

    for (int run = 0; run < 100; run++)
    {
        (void)fill_ns_per_key(entries); /* warmup */
    }
    for (int run = 0; run < 1000; run++)
    {
        double per_key = fill_ns_per_key(entries);
        total += per_key;
        if (per_key < best)
        {
            best = per_key;
        }
    }

    printf("uthash insert, per key (grows): avg %.2f ns, min %.2f ns (n=1000)\n\n",
           total / 1000.0, best);

    for (int i = 0; i < BENCH_KEYS; i++)
    {
        free(entries[i]);
    }
}

UBENCH(uthash, find_present)
{
    bench_entry *found = find_key("key100");
    UBENCH_DO_NOTHING((void *)&found);
}

UBENCH(uthash, find_absent)
{
    bench_entry *found = find_key("nosuchkey");
    UBENCH_DO_NOTHING((void *)&found);
}

/* uthash's own hash of the same key, for scale against the other two
   benchmarks' hash lines. HASH_FUNCTION is the library's default
   (Jenkins one-at-a-time) unless overridden. */
UBENCH(uthash, hash_short_key)
{
    unsigned hashv = hash_key("key100", 6);
    UBENCH_DO_NOTHING((void *)&hashv);
}

int main(const int argc, const char *const argv[static argc + 1])
{
    dbj_clintro(DBJ_APP_NAME, DBJ_APP_VERSION);

    fill_table();
    printf("%u keys in the table\n\n", HASH_COUNT(table));

    /* Apart from the benchmarks below -- see the note at the top. The
       entries are allocated outside the timed region, so this is
       uthash's insert and not the allocator's. */
    bench_insert();

    int failed = ubench_main(argc, argv);

    HASH_CLEAR(hh, table);
    return failed;
}
