#pragma once
/*
    2026AUG29       (c) dbj@dbj.org

    dbj_hash_key.h -- the key, and the slot handle that holds it.
    One discriminated union:

        HashKey         WHAT the key is: KT_ORDINAL, KT_STRING or
                        KT_SLICE. Adding a key type is adding a case
                        here and an arm to each of the two operations
                        below.

    The state of a slot -- EMPTY, NULL or VALUE -- used to live here too,
    on a HashKeyHandle. It belongs to the slot rather than to the key, and
    it now sits in the map's index array as dbj_slot_state. Nothing on
    this side knows a map exists.

    The map touches key internals nowhere. It goes through the two
    operations DBJ_MAKE_HASHMAP requires of every key type, named after
    the type:

        HashKey_hash    the key's hash, whatever kind it is
        HashKey_equal   two keys of the same kind, compared

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

/* A key, or the reason there is not one. Only one operation here can
   fail, so only one returns a result. */
DBJ_MAKERESULT(HashKey);

/* The OK arm. Valid only once dbj_result_is_ok() has said so. */
#define dbj_result_hash_key(result_) ((result_).HashKey_OK.my_value)

/* This side's result subtypes. One failure, so far. It does not simply
   reuse dbj_arena_result_type: the arena's codes belong to the arena's
   result type, and a caller of hash_key_slice_copy should not have to
   know which allocator sits underneath to read the failure. */
typedef enum : unsigned short
{
    HASH_KEY_ERR_NONE = 0,
    HASH_KEY_ERR_ARENA_EXHAUSTED,
} hash_key_result_type;

/* The subtype of a key failure. Meaningful only after
   dbj_result_is_err(). */
#define hash_key_result_type_of(result_) \
    ((hash_key_result_type)dbj_result_code(HashKey, (result_)))

/* Copy text into an arena and point a key at the copy. The arena
   answers the lifetime question the slice kind raises: the text lives
   as long as the arena does, and the caller chooses an arena that
   outlives the map.

   This used to hand back a zero-length KT_SLICE key when the arena was
   exhausted, which was indefensible: that is a perfectly ordinary
   looking key, and the caller had no way to tell it apart from a key
   whose text really is empty. It would then be inserted, and every
   other exhausted key would collide with it. A silent wrong answer.

   The failure is reported now. The arena's own message is passed
   through rather than reworded -- it already says what went wrong, and
   the location field will name this function as where it surfaced. */
[[nodiscard]] static inline HashKeyResult hash_key_slice_copy(dbj_arena *arena, const char *text)
{
    ptrdiff_t length = (ptrdiff_t)strlen(text);
    dbj_arena_result block = dbj_arena_new(arena, length, char);
    if (dbj_result_is_err(block))
    {
        return HashKey_make_err_coded(__func__,
                                      dbj_arena_result_message(block),
                                      HASH_KEY_ERR_ARENA_EXHAUSTED);
    }
    char *copy = dbj_arena_ptr(block, char);
    memcpy(copy, text, (size_t)length);
    return HashKey_make_ok((HashKey){.kind = KT_SLICE, .slice = {copy, length}});
}

/* ------------------------------------------------------------------
   The two operations every key kind must answer
   ------------------------------------------------------------------ */

/* FNV-1a over the whole fixed buffer, trailing zeros included: the
   buffer is zero-filled by the factory, so two equal strings hash the
   same without a strlen first.

   Eight bytes at a time, not one. FNV-1a's multiply is a dependency
   chain -- each round needs the previous round's product -- so a
   32-byte buffer costs 32 imuls back to back, and imul latency is what
   is being paid, not throughput. Measured: 33ns for the byte loop, out
   of 65ns for a whole string-key get. Four word rounds cost four.

   memcpy for the load, not a cast: KeyString's data is unsigned char
   with alignment 1, and a uint64_t* through it is both misaligned and
   an aliasing violation. GCC emits a single mov.

   The tail loop is for a KeyString whose capacity is not a multiple of
   eight -- str32 has none, but KeyString is configurable.

   The finaliser is not decoration. Word rounds diffuse a byte's bits
   upward only, so without it two keys differing in one low byte land
   in neighbouring buckets. The map would hide that (dbj_hashmap_mix
   runs splitmix64 over whatever it gets), but this function is public
   and its result must stand on its own. */
[[gnu::const]] static inline uint64_t hash_key_string_hash(KeyString text)
{
    uint64_t hash = 0xcbf29ce484222325u;
    size_t i = 0;

    for (; i + sizeof(uint64_t) <= sizeof(text.data); i += sizeof(uint64_t))
    {
        uint64_t word;
        memcpy(&word, text.data + i, sizeof word);
        hash ^= word;
        hash *= 0x00000100000001b3u;
    }
    for (; i < sizeof(text.data); i++)
    {
        hash ^= text.data[i];
        hash *= 0x00000100000001b3u;
    }

    hash ^= hash >> 32;
    hash *= 0x00000100000001b3u;
    hash ^= hash >> 32;
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
   key kind is added.

   Named HashKey_hash because DBJ_MAKE_HASHMAP(HashKey, ...) pastes that
   name -- the map calls it directly rather than through a pointer stored
   in the key. */
[[gnu::pure]] static inline uint64_t HashKey_hash(HashKey key)
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

   The hash places a key; this decides whether the key found there is the
   one being searched for. Both are needed, and neither replaces the
   other.

   [[gnu::pure]] for the same reason as HashKey_hash. */
[[gnu::pure]] static inline bool HashKey_equal(HashKey lhs, HashKey rhs)
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

