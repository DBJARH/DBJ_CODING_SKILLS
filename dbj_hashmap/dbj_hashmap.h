#pragma once
/*
    2026AUG29       (c) dbj@dbj.org

    dbj_hashmap.h -- flat, fixed-capacity, open-addressing hash map
    with an ordinal key. Header only.

        #define DBJ_MAKERESULT_IMPLEMENTATION   // in exactly one .c
        #include <dbj_hashmap.h>

    That one #define is inherited from toplevel/dbj_result.h, whose
    factories are ordinary functions and so belong to a single
    translation unit. Everything this header adds is `static inline`.

    Two ways it departs from the usual string-keyed table:

      1. The key is ordinal, not text. KeyType is its own hash -- there
         is no string to store and none to compare, and a collision
         cannot happen, because the key is itself rather than a digest
         of itself.

      2. Absence is a state of the slot, not a sentinel value. A slot's
         HashKey is EMPTY, NULL or VALUE -- the three states a single
         SQL cell has. Nothing infers "unused" from all-zero bytes, so
         any value at all is legal to store.

    Every operation returns HashMapElementResult. A key that is simply
    not there is still OK, carrying an element whose key.id is
    HK_EMPTY: absence is an answer, not a failure. ERR is reserved for
    the map being unable to answer at all -- capacity exhausted.

    Configure before including, or take the defaults:

        KeyType             the key, an ordinal type (unsigned int)
        DBJ_HASHMAP_SLOTS   slot count, a power of two (1024)
*/
#include <dbj_required_compile_time.h>

#include "dbj_arena.h"
#include "dbj_hash_string.h"

#include <stdint.h>

/* Slot count. A plain constant, not 1 << N: the shift says nothing the
   number does not, and the number is what a reader wants to see. It
   must be a power of two -- the probe below relies on it. */
#ifndef DBJ_HASHMAP_SLOTS
#define DBJ_HASHMAP_SLOTS 1024
#endif

/* The index mask, and the count of high bits the probe step is taken
   from. Both follow from the slot count; stated once, here. */
#define DBJ_HASHMAP_MASK (DBJ_HASHMAP_SLOTS - 1)
#define DBJ_HASHMAP_EXP 10

typedef struct
{
    HashMapElement slots[DBJ_HASHMAP_SLOTS];
} dbj_hashmap;

/* Mix an ordinal key into a well-distributed 64-bit value.

   Consecutive keys (0, 1, 2, ...) would otherwise land in consecutive
   slots -- fine for the index, but it leaves the probe step below
   correlated. This is splitmix64's finaliser: cheap, and it makes the
   high bits, where the step comes from, as good as the low ones. */
static inline uint64_t dbj_hashmap_mix(KeyType key, uint64_t seed)
{
    uint64_t hash = (uint64_t)key + seed;
    hash ^= hash >> 30;
    hash *= 0xbf58476d1ce4e5b9u;
    hash ^= hash >> 27;
    hash *= 0x94d049bb133111ebu;
    hash ^= hash >> 31;
    return hash;
}

/* Where a slot search ended. index means something only when found is
   true; when the whole table has been visited with no match and no
   free slot, found is false and the caller reports ERR. */
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
   That bound is the loop's real termination condition: without it, a
   lookup in a *full* table (no match, no free slot) never returns, and
   that hits reads as well as writes.

   The map's own address seeds the mix. ASLR randomises it, so an
   attacker cannot precompute keys that collide, and no seed has to be
   stored anywhere.

   HK_NULL does not stop the search: a null-valued entry is a present
   key, so probing continues past it exactly as past a value. */
static inline dbj_hashmap_probe dbj_hashmap_slot(const dbj_hashmap *map, KeyType key)
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

/* Read. The map is not modified and the element comes back as a copy,
   so the caller holds no pointer into the map.

   A key that is not present is not a failure: the result is OK and the
   element's key.id is HK_EMPTY. ERR means the map could not answer. */
static inline HashMapElementResult dbj_hashmap_get(const dbj_hashmap *map, KeyType key)
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
static inline HashMapElementResult dbj_hashmap_set(dbj_hashmap *map, KeyType key, HashString value)
{
    dbj_hashmap_probe probe = dbj_hashmap_slot(map, key);
    if (!probe.found)
    {
        return HashMapElement_make_err(__func__, "hashmap capacity exhausted");
    }
    map->slots[probe.index] = (HashMapElement){.key = hash_key_value(key), .val = value};
    return HashMapElement_make_ok(map->slots[probe.index]);
}

/* Write a key with no value -- the SQL NULL of this map. The key stays
   present, so probing does not stop at it and a later get reports
   HK_NULL rather than HK_EMPTY. */
static inline HashMapElementResult dbj_hashmap_set_null(dbj_hashmap *map, KeyType key)
{
    dbj_hashmap_probe probe = dbj_hashmap_slot(map, key);
    if (!probe.found)
    {
        return HashMapElement_make_err(__func__, "hashmap capacity exhausted");
    }
    map->slots[probe.index] = (HashMapElement){.key = hash_key_null(key)};
    return HashMapElement_make_ok(map->slots[probe.index]);
}

/* How many slots hold a key -- HK_VALUE and HK_NULL alike. */
static inline ptrdiff_t dbj_hashmap_count(const dbj_hashmap *map)
{
    ptrdiff_t used = 0;
    for (ptrdiff_t i = 0; i < DBJ_HASHMAP_SLOTS; i++)
    {
        used += map->slots[i].key.id != HK_EMPTY;
    }
    return used;
}
