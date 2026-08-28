/*
    2026AUG28       (c) dbj@dbj.org

    dbj_str for Wellons -- an ordinal-keyed hash map
    ------------------------------------------------
    A second take on dbj-arena-hashmap-hashtrie.c in this folder. Same
    arena, same open-address table, but nothing points anywhere: keys
    and values are owned values, not views into the arena.

    Two ways it departs from Wellons rather than merely renaming him:

      1. The key is ordinal, not text. Wellons' Env and FlatEnv are
         string-to-string, so a key has to be stored and compared.
         KeyType is its own hash -- no string to store, none to
         compare, and a collision is impossible because the key is
         itself rather than a digest of itself.

      2. Absence is a state of the slot, not a sentinel value. A slot's
         HashKey is EMPTY, NULL or VALUE -- the three states a single
         SQL cell has. Nothing has to infer "unused" from all-zero
         bytes, so any value at all is a legal thing to store.

    The types live in dbj_hash_string.h beside this file; only the map
    is here. Lookups return HashMapElementResult: OK carries the
    element, and a key that is simply not there is still OK, holding an
    element whose key.id is HK_EMPTY. ERR is reserved for the map being
    unable to answer -- capacity exhausted.
*/
#include <dbj_required_compile_time.h>

/* dbj_result.h's factories are defined in exactly one translation
   unit; this is it. Must precede dbj_hash_string.h, which invokes
   DBJ_MAKERESULT. */
#define DBJ_MAKERESULT_IMPLEMENTATION
#include "dbj_hash_string.h"

#define DBJ_NANOBENCH_IMPLEMENTATION
#include <dbj_nanobench.h>

#include <dbj_clintro.h>
#include <dbj_defer.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DBJ_APP_NAME "dbj_str_4_welons"
#define DBJ_APP_VERSION "0.5.0"

/* -------------------------------------------------------------------
   dbj_arena -- bump allocator
   -------------------------------------------------------------------
   Still needed: the map is large and has to live somewhere. What
   changed from the slice version is that nothing inside the map points
   back into it, so the arena's lifetime constrains only the map
   itself, never the values handed out of it. */

typedef struct
{
    char *beg;
    char *end;
} dbj_arena;

/* Factory: wrap an already-owned block. Evaluates block_ twice, so
   pass a variable, not a call. */
#define dbj_arena_make(block_, size_) \
    ((dbj_arena){(block_), (block_) + (size_)})

#define dbj_arena_new(arena_, count_, type_) \
    (type_ *)dbj_arena_alloc((arena_), (count_), sizeof(type_), _Alignof(type_))

/* Bump `count` objects of `size` bytes off the front, aligned and
   zeroed. Zeroed is what makes a fresh map an empty map: HK_EMPTY is
   the zero of hk_id.

   Returns nullptr when the block cannot satisfy the request; every
   caller checks. */
static void *dbj_arena_alloc(dbj_arena *arena, ptrdiff_t count, ptrdiff_t size, ptrdiff_t align)
{
    /* bytes needed to round beg up to `align` -- align is a power of two */
    ptrdiff_t pad = -(uintptr_t)arena->beg & (align - 1);
    /* division, not multiplication, so the check itself cannot overflow */
    if (count >= (arena->end - arena->beg - pad) / size)
    {
        return nullptr;
    }
    void *result = arena->beg + pad;
    arena->beg += pad + count * size;
    return memset(result, 0, count * size);
}

/* -------------------------------------------------------------------
   dbj_hashmap -- flat, fixed capacity, open addressing
   ------------------------------------------------------------------- */

/* Slot count. A plain constant, not 1 << N: the shift said nothing the
   number does not, and the number is what a reader wants to see. The
   probe below needs a power of two, which this is. */
#define DBJ_HASHMAP_SLOTS 1024

/* The index mask, and the number of high bits the probe step is taken
   from -- both follow from the slot count and are stated once here. */
#define DBJ_HASHMAP_MASK (DBJ_HASHMAP_SLOTS - 1)
#define DBJ_HASHMAP_EXP 10

typedef struct
{
    HashMapElement slots[DBJ_HASHMAP_SLOTS];
} dbj_hashmap;

/* Mix an ordinal key into a well-distributed 64-bit value.

   Consecutive keys (0, 1, 2, ...) would otherwise land in consecutive
   slots, which is fine for the index but leaves the step below
   correlated. This is splitmix64's finaliser: cheap, and it makes the
   high bits -- where the step comes from -- as good as the low ones. */
static uint64_t dbj_hashmap_mix(KeyType key, uint64_t seed)
{
    uint64_t hash = (uint64_t)key + seed;
    hash ^= hash >> 30;
    hash *= 0xbf58476d1ce4e5b9u;
    hash ^= hash >> 27;
    hash *= 0x94d049bb133111ebu;
    hash ^= hash >> 31;
    return hash;
}

/* Where a slot search ended. index is meaningful only when found is
   true; when the whole table has been visited without a match and
   without a free slot, found is false and the caller reports ERR. */
typedef struct
{
    bool found;
    ptrdiff_t index;
} dbj_hashmap_probe;

/* Probe for `key`: the slot already holding it, or the first slot free
   to claim.

   Double hashing: low bits index, high bits supply an odd step. Odd is
   coprime with a power-of-two table size, so probing visits every slot
   exactly once -- which is why the loop is bounded by the slot count.
   Without that bound a lookup in a *full* table (no match, no free
   slot) never terminates, and that hits reads as well as writes.

   The map's own address seeds the mix. ASLR randomises it, so an
   attacker cannot precompute keys that collide, and no seed has to be
   stored anywhere.

   HK_NULL does not stop the search: a null-valued entry is a present
   key, so probing continues past it exactly as it would past a value. */
static dbj_hashmap_probe dbj_hashmap_slot(const dbj_hashmap *map, KeyType key)
{
    uint64_t hash = dbj_hashmap_mix(key, (uintptr_t)map);
    uint32_t step = (uint32_t)(hash >> (64 - DBJ_HASHMAP_EXP)) | 1;
    uint32_t index = (uint32_t)hash;

    for (int probes = 0; probes < DBJ_HASHMAP_SLOTS; probes++)
    {
        index = (index + step) & DBJ_HASHMAP_MASK;
        const HashKey *slot_key = &map->slots[index].key;

        switch (slot_key->id)
        {
        case HK_EMPTY:
            /* never written: the key is not here, and this is where it
               would go */
            return (dbj_hashmap_probe){.found = true, .index = (ptrdiff_t)index};
        case HK_NULL:
        case HK_VALUE:
            if (slot_key->val.key == key)
            {
                return (dbj_hashmap_probe){.found = true, .index = (ptrdiff_t)index};
            }
            break;
        }
    }
    return (dbj_hashmap_probe){0}; /* every slot visited, table is full */
}

/* Read. The map is not modified and the element comes back as a copy,
   so the caller holds no pointer into the map.

   A key that is not present is not a failure: the result is OK, and
   the element's key.id is HK_EMPTY. ERR means the map could not
   answer at all. */
static HashMapElementResult dbj_hashmap_get(const dbj_hashmap *map, KeyType key)
{
    dbj_hashmap_probe probe = dbj_hashmap_slot(map, key);
    if (!probe.found)
    {
        return HashMapElement_make_err(__func__, "hashmap capacity exhausted");
    }
    return HashMapElement_make_ok(map->slots[probe.index]);
}

/* Write a key with a value. Inserts, or replaces the value of a key
   already present. Returns the element as stored. */
static HashMapElementResult dbj_hashmap_set(dbj_hashmap *map, KeyType key, HashString value)
{
    dbj_hashmap_probe probe = dbj_hashmap_slot(map, key);
    if (!probe.found)
    {
        return HashMapElement_make_err(__func__, "hashmap capacity exhausted");
    }
    map->slots[probe.index] = (HashMapElement){.key = hash_key_value(key), .val = value};
    return HashMapElement_make_ok(map->slots[probe.index]);
}

/* Write a key with no value -- the SQL NULL of this map. The key is
   present, so probing does not stop at it and a later get reports
   HK_NULL rather than HK_EMPTY. */
static HashMapElementResult dbj_hashmap_set_null(dbj_hashmap *map, KeyType key)
{
    dbj_hashmap_probe probe = dbj_hashmap_slot(map, key);
    if (!probe.found)
    {
        return HashMapElement_make_err(__func__, "hashmap capacity exhausted");
    }
    map->slots[probe.index].key = hash_key_null();
    map->slots[probe.index].key.val.key = key; /* NULL still remembers which key */
    map->slots[probe.index].val = (HashString){0};
    return HashMapElement_make_ok(map->slots[probe.index]);
}

/* How many slots hold a key -- HK_VALUE or HK_NULL alike. */
static ptrdiff_t dbj_hashmap_count(const dbj_hashmap *map)
{
    ptrdiff_t used = 0;
    for (ptrdiff_t i = 0; i < DBJ_HASHMAP_SLOTS; i++)
    {
        used += map->slots[i].key.id != HK_EMPTY;
    }
    return used;
}

/* -------------------------------------------------------------------
   Demo
   ------------------------------------------------------------------- */

#define DBJ_DEMO_COUNT 256
#define DBJ_DEMO_PROBE 100

/* Name the three states for printing. No `default`: adding an hk_id
   must break this build. */
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
static void report(const char *what, HashMapElementResult result)
{
    switch (result.tag)
    {
    case DBJ_RESULT_OK:
    {
        HashMapElement element = result.HashMapElement_OK.my_value;
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
               result.HashMapElement_ERR.location,
               result.HashMapElement_ERR.message);
        break;
    }
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

    report("get 100", dbj_hashmap_get(map, DBJ_DEMO_PROBE));
    report("get 9999", dbj_hashmap_get(map, 9999));

    /* overwrite, then the same key as an explicit NULL */
    (void)dbj_hashmap_set(map, DBJ_DEMO_PROBE, hash_string_64("overwritten"));
    report("set 100 + get", dbj_hashmap_get(map, DBJ_DEMO_PROBE));

    (void)dbj_hashmap_set_null(map, DBJ_DEMO_PROBE);
    report("null 100 + get", dbj_hashmap_get(map, DBJ_DEMO_PROBE));

    printf("  %-18s %td of %d slots\n", "occupied",
           dbj_hashmap_count(map), DBJ_HASHMAP_SLOTS);
}

/* -------------------------------------------------------------------
   Benchmarks
   ------------------------------------------------------------------- */

/* One map, filled once, measured many times. Static rather than
   arena-allocated so the benchmark measures the map and not the
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
