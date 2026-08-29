#pragma once
/*
    2026AUG29       (c) dbj@dbj.org

    dbj_hashmap.h -- flat, fixed-capacity, open-addressing hash map
    with an ordinal key. Header only, all static inline.

        #define DBJ_MAKERESULT_IMPLEMENTATION   // in exactly one .c
        #include <dbj_hashmap.h>

    Two departures from the usual string-keyed table:

      1. The key is a HashKey -- a discriminated union over key kinds
         (ordinal, string, whatever is added). The map never touches a
         key's internals; it asks hash_key_hash and hash_key_equal, so
         a new key kind is a case in dbj_hash_key.h and nothing here.

      2. Absence is a state of the slot, not a sentinel value, so any
         value at all is legal to store.

    Every operation returns HashMapElementResult -- see
    dbj_hashmap_element_result.h.

    Configure before including, or take the defaults:

        KeyOrdinal          the ordinal key type (unsigned int)
        KeyString           the string key type (str32)
        DBJ_HASHMAP_SLOTS   slot count, a power of two (1024)
*/
#include <dbj_required_compile_time.h>

#include "dbj_arena.h"
#include "dbj_hash_key.h"
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

/* One entry of the index: the slot's state and the hash that put the
   key there. Sixteen bytes, four of them padding after the enum. */
typedef struct
{
    hk_id id;
    uint64_t hash;
} dbj_hashmap_index;

/* Three arrays, not one array of HashMapElement.

   The probe reads the state and the hash and nothing else, but a
   HashMapElement is 184 bytes -- 132 of them the value -- so an array
   of them makes the probe stride 184 bytes through 184 KB, dragging a
   value into L1 at every step to look at four bytes of tag. Split, the
   probe walks 16 KB, which stays in L1 whatever the load, and touches
   a key only when the stored hash already matched.

   Storing the hash is the other half. Without it every occupied slot
   on the probe path costs a hash_key_equal -- a 32-byte memcmp for a
   string key, a pointer dereference for a slice. With it, a mismatch
   is one 64-bit compare and hash_key_equal runs only on a candidate.

   Total memory is unchanged; what changed is which of it the probe
   has to walk. A zeroed dbj_hashmap is still an empty map: hk_id's
   zero is HK_EMPTY. */
typedef struct
{
    dbj_hashmap_index index[DBJ_HASHMAP_SLOTS];
    HashKey keys[DBJ_HASHMAP_SLOTS];
    HashString vals[DBJ_HASHMAP_SLOTS];
} dbj_hashmap;

/* The three arrays reassembled into the element the caller sees.
   HashMapElement is the shape of an answer, not the shape of storage
   -- those were one type until the probe's cost was measured.

   always_inline, here and on the three operations below, is measured
   and not a preference. Assembling an element out of three arrays
   costs more inliner budget than reading one struct did, and past the
   threshold GCC put dbj_hashmap_set out of line -- at which point its
   132-byte HashString argument had to be copied onto the stack at
   every call site, an expense that does not exist when the call is
   inlined. Insert went 5ns to 27ns per key on that alone. These are
   header-only operations over a caller-owned map; out of line was
   never the intent. */
[[nodiscard, gnu::always_inline]] static inline HashMapElement dbj_hashmap_element_at(const dbj_hashmap *map, ptrdiff_t index)
{
    return (HashMapElement){
        .key = {.id = map->index[index].id, .val = map->keys[index]},
        .val = map->vals[index]};
}

/* Consecutive ordinal keys (0, 1, 2, ...) land in consecutive slots --
   fine for the index, but it leaves the probe step correlated.
   splitmix64's finaliser is cheap and makes the high bits, where the
   step comes from, as good as the low ones. A string key arrives
   already hashed by FNV-1a, and mixing it again costs nothing.

   [[gnu::const]]: the result depends on the arguments and nothing
   else, so repeated calls fold into one. Not decoration -- at
   -O1 -fno-inline two identical calls emit a single `call`. (The
   weaker [[gnu::pure]] is for a function that reads memory.) */
[[gnu::const]] static inline uint64_t dbj_hashmap_mix(uint64_t key, uint64_t seed)
{
    uint64_t hash = key + seed;
    hash ^= hash >> 30;
    hash *= 0xbf58476d1ce4e5b9u;
    hash ^= hash >> 27;
    hash *= 0x94d049bb133111ebu;
    hash ^= hash >> 31;
    return hash;
}

/* index and hash mean something only when found is true. hash is
   carried out because a set must write it into the slot, and rehashing
   the key to do that would throw away the work the probe just did. */
typedef struct
{
    bool found;
    uint32_t index; /* uint32_t, not ptrdiff_t, to hold this to 16 bytes:
                       a 24-byte struct returns through memory, a 16-byte
                       one of integer fields returns in rax:rdx */
    uint64_t hash;
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
   key, so probing continues past it as it would past a value.

   The stored hash is compared first. It cannot prove two keys equal --
   a collision is still a collision -- so hash_key_equal decides, but
   it now runs only on slots whose hash already matched. */
[[nodiscard]] static inline dbj_hashmap_probe dbj_hashmap_slot(const dbj_hashmap *map, HashKey key)
{
    uint64_t hash = dbj_hashmap_mix(hash_key_hash(key), (uintptr_t)map);
    uint32_t step = (uint32_t)(hash >> (64 - DBJ_HASHMAP_EXP)) | 1;
    uint32_t index = (uint32_t)hash;

    for (int probes = 0; probes < DBJ_HASHMAP_SLOTS; probes++)
    {
        index = (index + step) & DBJ_HASHMAP_MASK;
        const dbj_hashmap_index *slot = &map->index[index];

        switch (slot->id)
        {
        case HK_EMPTY:
            /* never written: the key is not here, and here is where it
               would go */
            return (dbj_hashmap_probe){.found = true, .index = index, .hash = hash};
        case HK_NULL:
        case HK_VALUE:
            if (slot->hash == hash && hash_key_equal(map->keys[index], key))
            {
                return (dbj_hashmap_probe){.found = true, .index = index, .hash = hash};
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
[[nodiscard, gnu::always_inline]] static inline HashMapElementResult dbj_hashmap_get(const dbj_hashmap *map, HashKey key)
{
    dbj_hashmap_probe probe = dbj_hashmap_slot(map, key);
    if (!probe.found)
    {
        return HashMapElement_make_err(__func__, "hashmap capacity exhausted");
    }
    return HashMapElement_make_ok(dbj_hashmap_element_at(map, probe.index));
}

/* Insert, or replace the value of a key already present. */
[[nodiscard, gnu::always_inline]] static inline HashMapElementResult dbj_hashmap_set(dbj_hashmap *map, HashKey key, HashString value)
{
    dbj_hashmap_probe probe = dbj_hashmap_slot(map, key);
    if (!probe.found)
    {
        return HashMapElement_make_err(__func__, "hashmap capacity exhausted");
    }
    map->index[probe.index] = (dbj_hashmap_index){.id = HK_VALUE, .hash = probe.hash};
    map->keys[probe.index] = key;
    map->vals[probe.index] = value;
    return HashMapElement_make_ok(dbj_hashmap_element_at(map, probe.index));
}

/* The SQL NULL of this map. The key stays present, so probing does not
   stop at it and a later get reports HK_NULL, not HK_EMPTY. */
[[nodiscard, gnu::always_inline]] static inline HashMapElementResult dbj_hashmap_set_null(dbj_hashmap *map, HashKey key)
{
    dbj_hashmap_probe probe = dbj_hashmap_slot(map, key);
    if (!probe.found)
    {
        return HashMapElement_make_err(__func__, "hashmap capacity exhausted");
    }
    map->index[probe.index] = (dbj_hashmap_index){.id = HK_NULL, .hash = probe.hash};
    map->keys[probe.index] = key;
    map->vals[probe.index] = (HashString){0};
    return HashMapElement_make_ok(dbj_hashmap_element_at(map, probe.index));
}

/* Slots holding a key, HK_VALUE and HK_NULL alike. */
[[nodiscard]] static inline ptrdiff_t dbj_hashmap_count(const dbj_hashmap *map)
{
    ptrdiff_t used = 0;
    for (ptrdiff_t i = 0; i < DBJ_HASHMAP_SLOTS; i++)
    {
        used += map->index[i].id != HK_EMPTY;
    }
    return used;
}
