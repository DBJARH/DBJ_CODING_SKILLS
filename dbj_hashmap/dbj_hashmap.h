#pragma once
/*
    2026AUG29       (c) dbj@dbj.org

    dbj_hashmap.h -- flat, fixed-capacity, open-addressing hash map
    with an ordinal key. Header only, all static inline.

        #define DBJ_MAKERESULT_IMPLEMENTATION   // in exactly one .c
        #include <dbj_hashmap.h>

    Two departures from the usual string-keyed table:

      1. The key is ordinal. KeyType is its own hash -- nothing to
         store, nothing to compare, and no collision is possible,
         because the key is itself rather than a digest of itself.

      2. Absence is a state of the slot, not a sentinel value, so any
         value at all is legal to store.

    Every operation returns HashMapElementResult -- see
    dbj_hashmap_element_result.h.

    Configure before including, or take the defaults:

        KeyType             the key, an ordinal type (unsigned int)
        DBJ_HASHMAP_SLOTS   slot count, a power of two (1024)
*/
#include <dbj_required_compile_time.h>

#include "dbj_arena.h"
#include "dbj_hashmap_element_result.h"

#include <stdint.h>

/* Must be a power of two -- the probe relies on it. A plain constant,
   not 1 << N: the shift says nothing the number does not. */
#ifndef DBJ_HASHMAP_SLOTS
#define DBJ_HASHMAP_SLOTS 1024
#endif

/* Both follow from the slot count; stated once, here. */
#define DBJ_HASHMAP_MASK (DBJ_HASHMAP_SLOTS - 1)
#define DBJ_HASHMAP_EXP 10

typedef struct
{
    HashMapElement slots[DBJ_HASHMAP_SLOTS];
} dbj_hashmap;

/* Consecutive keys (0, 1, 2, ...) land in consecutive slots -- fine
   for the index, but it leaves the probe step correlated. splitmix64's
   finaliser is cheap and makes the high bits, where the step comes
   from, as good as the low ones.

   [[gnu::const]]: the result depends on the arguments and nothing
   else, so repeated calls fold into one. Not decoration -- at
   -O1 -fno-inline two identical calls emit a single `call`. (The
   weaker [[gnu::pure]] is for a function that reads memory.) */
[[gnu::const]] static inline uint64_t dbj_hashmap_mix(KeyType key, uint64_t seed)
{
    uint64_t hash = (uint64_t)key + seed;
    hash ^= hash >> 30;
    hash *= 0xbf58476d1ce4e5b9u;
    hash ^= hash >> 27;
    hash *= 0x94d049bb133111ebu;
    hash ^= hash >> 31;
    return hash;
}

/* index means something only when found is true. */
typedef struct
{
    bool found;
    ptrdiff_t index;
} dbj_hashmap_probe;

/* The slot already holding `key`, or the first free to claim.

   Double hashing: low bits index, high bits supply an odd step, which
   is coprime with a power-of-two size and so visits every slot exactly
   once. That is why the loop is bounded by the slot count -- it is the
   real termination condition. Without it a lookup in a *full* table
   (no match, no free slot) never returns, and that hits reads too.

   The map's own address seeds the mix, so ASLR randomises it and no
   seed is stored.

   HK_NULL does not stop the search: a null-valued entry is a present
   key, so probing continues past it as it would past a value. */
[[nodiscard]] static inline dbj_hashmap_probe dbj_hashmap_slot(const dbj_hashmap *map, KeyType key)
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
            /* never written: the key is not here, and here is where it
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
    return (dbj_hashmap_probe){0}; /* every slot visited: the table is full */
}

/* The element comes back as a copy, so the caller holds no pointer
   into the map. A key that is not present is not a failure: OK, with
   key.id HK_EMPTY. ERR means the map could not answer.

   [[nodiscard]] throughout: the result is the only report that the map
   was full, so dropping it must be spelled (void). */
[[nodiscard]] static inline HashMapElementResult dbj_hashmap_get(const dbj_hashmap *map, KeyType key)
{
    dbj_hashmap_probe probe = dbj_hashmap_slot(map, key);
    if (!probe.found)
    {
        return HashMapElement_make_err(__func__, "hashmap capacity exhausted");
    }
    return HashMapElement_make_ok(map->slots[probe.index]);
}

/* Insert, or replace the value of a key already present. */
[[nodiscard]] static inline HashMapElementResult dbj_hashmap_set(dbj_hashmap *map, KeyType key, HashString value)
{
    dbj_hashmap_probe probe = dbj_hashmap_slot(map, key);
    if (!probe.found)
    {
        return HashMapElement_make_err(__func__, "hashmap capacity exhausted");
    }
    map->slots[probe.index] = (HashMapElement){.key = hash_key_value(key), .val = value};
    return HashMapElement_make_ok(map->slots[probe.index]);
}

/* The SQL NULL of this map. The key stays present, so probing does not
   stop at it and a later get reports HK_NULL, not HK_EMPTY. */
[[nodiscard]] static inline HashMapElementResult dbj_hashmap_set_null(dbj_hashmap *map, KeyType key)
{
    dbj_hashmap_probe probe = dbj_hashmap_slot(map, key);
    if (!probe.found)
    {
        return HashMapElement_make_err(__func__, "hashmap capacity exhausted");
    }
    map->slots[probe.index] = (HashMapElement){.key = hash_key_null(key)};
    return HashMapElement_make_ok(map->slots[probe.index]);
}

/* Slots holding a key, HK_VALUE and HK_NULL alike. */
[[nodiscard]] static inline ptrdiff_t dbj_hashmap_count(const dbj_hashmap *map)
{
    ptrdiff_t used = 0;
    for (ptrdiff_t i = 0; i < DBJ_HASHMAP_SLOTS; i++)
    {
        used += map->slots[i].key.id != HK_EMPTY;
    }
    return used;
}
