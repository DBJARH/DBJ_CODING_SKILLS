/*
    2026AUG28       (c) dbj@dbj.org

    dbj_str for Wellons -- exercising the ordinal-keyed hash map
    ------------------------------------------------------------
    A second take on dbj-arena-hashmap-hashtrie.c in this folder. Same
    arena, same open-address table, but nothing points anywhere: keys
    and values are owned values, not views into the arena.

    The map itself no longer lives here. It was decoupled into
    ../dbj_hashmap/, which is a library folder; this file is one of its
    users, and holds only the demo and the benchmarks.

    What the demo shows is the part worth seeing: a lookup for a key
    that is not there comes back OK, not ERR, carrying an element whose
    key.id is HK_EMPTY. Absence is an answer. ERR is kept for the map
    being unable to answer at all.
*/
#include <dbj_required_compile_time.h>

/* dbj_result.h's factories are ordinary functions and belong to one
   translation unit; this is it. Must precede dbj_hashmap.h, which
   pulls in dbj_hash_string.h and its DBJ_MAKERESULT invocation. */
#define DBJ_MAKERESULT_IMPLEMENTATION
#include <dbj_hashmap.h>

#define DBJ_NANOBENCH_IMPLEMENTATION
#include <dbj_nanobench.h>

#include <dbj_clintro.h>
#include <dbj_defer.h>

#include <stdio.h>
#include <stdlib.h>

#define DBJ_APP_NAME "dbj_str_4_welons"
#define DBJ_APP_VERSION "0.5.0"

#define DBJ_DEMO_COUNT 256
#define DBJ_DEMO_PROBE 100

/* -------------------------------------------------------------------
   Demo
   ------------------------------------------------------------------- */

/* Name the three slot states for printing. No `default`: adding an
   hk_id must break this build -- see ../CLAUDE.md on -Wswitch-enum. */
static const char *hk_id_name(hk_id id)
{
    switch (id)
    {
    case HK_EMPTY:
        return "EMPTY";
    case HK_NULL:
        return "NULL";
    case HK_VALUE:
        return "VALUE";
    }
    __builtin_unreachable(); /* no fake return; the switch has no default */
}

/* Print whatever a lookup returned, in either arm. */
static void result_report(const char *what, HashMapElementResult result)
{
    switch (result.tag)
    {
    case DBJ_RESULT_OK:
    {
        HashMapElement element = dbj_result_element(result);
        printf("  %-18s OK    %-5s", what, hk_id_name(element.key.id));
        if (element.key.id == HK_VALUE)
        {
            printf(" \"%s\"", hash_string_text(&element.val));
        }
        printf("\n");
        break;
    }
    case DBJ_RESULT_ERR:
        printf("  %-18s ERR   %s: %s\n", what,
               dbj_result_location(result),
               dbj_result_message(result));
        break;
    }
    /* No __builtin_unreachable() here, unlike the value-returning
       switch above: this function is void, so control *does* reach the
       end normally after either arm breaks. Claiming otherwise is UB,
       and GCC acts on it -- it cost a segfault to find out. The
       missing `default` is still what makes a new tag a build error. */
}

/* Takes the arena by value: a scratch copy, whose allocations are gone
   when this returns. Note what is not a hazard -- every element
   printed here was copied out of the map, so it would stay valid even
   after the arena died. */
static void dbj_hashmap_demo(dbj_arena scratch)
{
    dbj_hashmap *map = dbj_arena_new(&scratch, 1, dbj_hashmap);
    if (!map)
    {
        printf("  arena too small for a dbj_hashmap\n");
        return;
    }

    char text[64];
    for (int i = 0; i < DBJ_DEMO_COUNT; i++)
    {
        snprintf(text, sizeof(text), "value%d", i);
        (void)dbj_hashmap_set(map, (KeyType)i, hash_string_32(text));
    }

    result_report("get 100", dbj_hashmap_get(map, DBJ_DEMO_PROBE));
    result_report("get 9999", dbj_hashmap_get(map, 9999));

    /* overwrite, then the same key again as an explicit NULL */
    (void)dbj_hashmap_set(map, DBJ_DEMO_PROBE, hash_string_64("overwritten"));
    result_report("set 100 + get", dbj_hashmap_get(map, DBJ_DEMO_PROBE));

    (void)dbj_hashmap_set_null(map, DBJ_DEMO_PROBE);
    result_report("null 100 + get", dbj_hashmap_get(map, DBJ_DEMO_PROBE));

    printf("  %-18s %td of %d slots\n", "occupied",
           dbj_hashmap_count(map), DBJ_HASHMAP_SLOTS);
}

/* -------------------------------------------------------------------
   Benchmarks
   ------------------------------------------------------------------- */

/* One map, filled once, measured many times. Static rather than
   arena-allocated so the benchmark measures the map, not the
   allocator. */
static dbj_hashmap bench_map;

static void run_benchmarks(void)
{
    for (int i = 0; i < DBJ_DEMO_COUNT; i++)
    {
        (void)dbj_hashmap_set(&bench_map, (KeyType)i, hash_string_32("v"));
    }

    /* a hit: probe, then copy the element out */
    DBJ_BENCH("hashmap get, key present", HashMapElementResult, {
        DBJ_NB_val = dbj_hashmap_get(&bench_map, DBJ_DEMO_PROBE);
    });

    /* a miss: probes until it meets an empty slot */
    DBJ_BENCH("hashmap get, key absent", HashMapElementResult, {
        DBJ_NB_val = dbj_hashmap_get(&bench_map, 9999);
    });

    /* the probe alone, without the element copy */
    DBJ_BENCH("hashmap probe only", dbj_hashmap_probe, {
        DBJ_NB_val = dbj_hashmap_slot(&bench_map, DBJ_DEMO_PROBE);
    });

    /* what a HashString costs to build, for scale against the above */
    DBJ_BENCH("hash_string_32 fill", HashString, {
        DBJ_NB_val = hash_string_32("value100");
    });
}

/* -------------------------------------------------------------------
   main
   ------------------------------------------------------------------- */

#define DBJ_ARENA_SIZE (16 * 1024 * 1024)

int main(int argc, char *argv[static argc + 1])
{
    (void)argv;
    dbj_clintro(DBJ_APP_NAME, DBJ_APP_VERSION);

    char *block = malloc(DBJ_ARENA_SIZE);
    if (!block)
    {
        fprintf(stderr, "%s: cannot allocate %d bytes\n", DBJ_APP_NAME, DBJ_ARENA_SIZE);
        return EXIT_FAILURE;
    }
    /* the one and only allocation in this program */
    defer { free(block); };

    dbj_hashmap_demo(dbj_arena_make(block, DBJ_ARENA_SIZE));

    printf("\n");
    run_benchmarks();

    return EXIT_SUCCESS;
}
