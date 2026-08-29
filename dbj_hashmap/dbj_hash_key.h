#pragma once
/*
    2026AUG29       (c) dbj@dbj.org

    dbj_hash_key.h -- the key, and the slot handle that holds it.
    Two discriminated unions on two orthogonal axes:

        HashKey         WHAT the key is: KT_ORDINAL, KT_STRING or
                        KT_SLICE. Adding a key type is adding a case
                        here and an arm to each of the two operations
                        below.
        HashKeyHandle   the STATE of the slot: EMPTY, NULL or VALUE,
                        the three states a single SQL cell has. Three
                        values, and it stays three.

    The map touches key internals nowhere. It goes through:

        hash_key_hash   the key's hash, whatever kind it is
        hash_key_equal  two keys of the same kind, compared

    Two of the three kinds hold their text. KT_SLICE points at text it
    does not hold -- see "The pointing key" below, which is the one
    thing a caller must read before using it.

    Configure before including, or take the defaults:

        KeyOrdinal      the ordinal key type (unsigned int)
        KeyString       the string key type, a DEFINE_DBJSTR_TYPE name
                        (str32)

    ------------------------------------------------------------------
    The pointing key

    KT_SLICE stores a dbj_str_slice: a pointer and a length. The map
    copies the slice, not the text, so the key is valid exactly as long
    as what it points at is. There is no copy and no 32-byte ceiling --
    a key of any length costs the same 16 bytes.

    Point it at a string literal, which outlives everything, or use
    hash_key_slice_copy to put the text in an arena that outlives the
    map. Point it at a stack buffer and the key is garbage the moment
    the frame goes.

    That is the trade this kind exists to show. KT_STRING holds its
    text and truncates at KeyString's capacity; KT_SLICE truncates
    nothing and holds nothing. Neither is the right answer in general.
    ------------------------------------------------------------------
*/
#include <dbj_required_compile_time.h>

#include "dbj_arena.h"

#include <dbj_str.h>
#include <dbj_str_slice.h>

#include <stdint.h>
#include <string.h>

/* str32 is declared here, not in dbj_hash_string.h, because KeyString
   defaults to it -- the key side must not depend on the value side
   being included first. The value side includes this header and
   declares the sizes it adds. */
DEFINE_DBJSTR_TYPE(str32, 32)

/* Ordinal: the key is its own hash, so nothing is stored beyond the
   number and no collision is possible. */
#ifndef KeyOrdinal
#define KeyOrdinal unsigned int
#endif

/* String: an alias to a DEFINE_DBJSTR_TYPE name, not a fresh
   instantiation -- str32 already exists, and a second 32-byte type
   would be a duplicate with a different name. */
#ifndef KeyString
#define KeyString str32
#endif

typedef enum
{
    KT_ORDINAL,
    KT_STRING,
    KT_SLICE
} kt_id;

/* kind says which member is live -- nothing more. The union is as
   large as KeyString, so every slot pays for the string key even in a
   map that never stores one. That is the price of one map type over
   all key types, and it is deliberate. */
typedef struct
{
    kt_id kind;
    union
    {
        KeyOrdinal ord;
        KeyString str;
        dbj_str_slice slice;
    };
} HashKey;

[[nodiscard]] static inline HashKey hash_key_ordinal(KeyOrdinal ordinal)
{
    return (HashKey){.kind = KT_ORDINAL, .ord = ordinal};
}

/* Truncates what does not fit, as the value-side fillers do. */
[[nodiscard]] static inline HashKey hash_key_string(const char *text)
{
    HashKey result = {.kind = KT_STRING, .str = {}};
    size_t length = strlen(text);
    if (length > sizeof(result.str.data))
    {
        length = sizeof(result.str.data); /* truncate */
    }
    memcpy(result.str.data, text, length);
    return result;
}

/* The map copies the slice, not the text. Whatever the slice points
   at must outlive the map -- see "The pointing key" at the top.
   Nothing here can check that. */
[[nodiscard]] static inline HashKey hash_key_slice(dbj_str_slice slice)
{
    return (HashKey){.kind = KT_SLICE, .slice = slice};
}

/* Copy text into an arena and point a key at the copy. The arena
   answers the lifetime question the slice kind raises: the text lives
   as long as the arena does, and the caller chooses an arena that
   outlives the map.

   A zero-length slice when the arena is exhausted -- dbj_arena_alloc
   returns nullptr rather than aborting, and this keeps that. */
[[nodiscard]] static inline HashKey hash_key_slice_copy(dbj_arena *arena, const char *text)
{
    ptrdiff_t length = (ptrdiff_t)strlen(text);
    char *copy = dbj_arena_new(arena, length, char);
    if (!copy)
    {
        return (HashKey){.kind = KT_SLICE, .slice = {}};
    }
    memcpy(copy, text, (size_t)length);
    return (HashKey){.kind = KT_SLICE, .slice = {copy, length}};
}

/* ------------------------------------------------------------------
   The two operations every key kind must answer
   ------------------------------------------------------------------ */

/* FNV-1a over the whole fixed buffer, trailing zeros included: the
   buffer is zero-filled by the factory, so two equal strings hash the
   same without a strlen first. */
[[gnu::const]] static inline uint64_t hash_key_string_hash(KeyString text)
{
    uint64_t hash = 0xcbf29ce484222325u;
    for (size_t i = 0; i < sizeof(text.data); i++)
    {
        hash ^= text.data[i];
        hash *= 0x00000100000001b3u;
    }
    return hash;
}

/* An ordinal is its own hash -- the map mixes it, so nothing is
   gained by hashing it twice.

   [[gnu::pure]], not [[gnu::const]]: KT_SLICE reads the bytes its
   pointer names, and `const` promises a function reads no memory at
   all. `pure` is the weaker promise that fits -- same arguments, same
   result, provided memory has not changed in between. The ordinal and
   string arms would still qualify as `const`; the attribute belongs to
   the function, not the arm, so the weakest arm sets it.

   No `default` case, so -Wswitch-enum -Werror names this site when a
   key kind is added. */
[[gnu::pure]] static inline uint64_t hash_key_hash(HashKey key)
{
    switch (key.kind)
    {
    case KT_ORDINAL:
        return (uint64_t)key.ord;
    case KT_STRING:
        return hash_key_string_hash(key.str);
    case KT_SLICE:
        /* seed 0: the map seeds its own mix with its address, so
           seeding twice buys nothing */
        return dbj_str_slice_hash(key.slice, 0);
    }
    __builtin_unreachable(); /* no fake return; the switch has no default */
}

/* Keys of different kinds are never equal, whatever their bits say.
   A KT_STRING and a KT_SLICE of the same text are two different keys
   -- deliberately: one holds its text and one points at text held
   elsewhere, and silently equating them would hide that.

   [[gnu::pure]] for the same reason as hash_key_hash. */
[[gnu::pure]] static inline bool hash_key_equal(HashKey lhs, HashKey rhs)
{
    if (lhs.kind != rhs.kind)
    {
        return false;
    }
    switch (lhs.kind)
    {
    case KT_ORDINAL:
        return lhs.ord == rhs.ord;
    case KT_STRING:
        return memcmp(lhs.str.data, rhs.str.data, sizeof(lhs.str.data)) == 0;
    case KT_SLICE:
        return dbj_str_slice_equals(lhs.slice, rhs.slice);
    }
    __builtin_unreachable(); /* no fake return; the switch has no default */
}

/* ------------------------------------------------------------------
   HashKeyHandle -- the state of the slot holding a key
   ------------------------------------------------------------------ */

/* EMPTY  nothing was ever stored here; the key member means nothing
   NULL   a key is present, but holds no value
   VALUE  a key with a value

   A zeroed handle is HK_EMPTY, which is what makes a zeroed map an
   empty map. */
typedef enum
{
    HK_EMPTY,
    HK_NULL,
    HK_VALUE
} hk_id;

/* Not a union: HK_NULL and HK_VALUE both carry the key, and HK_EMPTY
   simply does not read it. A union over "key" and "no key" would buy
   nothing and cost a second tag. */
typedef struct
{
    hk_id id;
    HashKey val;
} HashKeyHandle;

/* A zeroed handle is already HK_EMPTY, so this is for clarity at a
   call site, not for initialising. Hence [[maybe_unused]]. */
[[nodiscard, maybe_unused]] static inline HashKeyHandle hash_key_empty(void)
{
    return (HashKeyHandle){.id = HK_EMPTY};
}

/* HK_NULL still carries its key: the entry is present, so a probe must
   be able to match it. Only HK_EMPTY has no key. */
[[nodiscard]] static inline HashKeyHandle hash_key_null(HashKey key)
{
    return (HashKeyHandle){.id = HK_NULL, .val = key};
}

[[nodiscard]] static inline HashKeyHandle hash_key_value(HashKey key)
{
    return (HashKeyHandle){.id = HK_VALUE, .val = key};
}
