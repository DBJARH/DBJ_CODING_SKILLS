#pragma once
/*
    2026AUG30       (c) dbj@dbj.org

    dbj_make_hashmap.h -- DBJ_MAKE_HASHMAP(K, V) generates a flat,
    fixed-capacity, open-addressing hash map for one key type and one
    value type. Header only, all static inline.

        #define DBJ_MAKERESULT_IMPLEMENTATION   // in exactly one .c
        #include "dbj_make_hashmap.h"

        uint64_t short_hash (short);            // the caller writes these
        bool     short_equal(short, short);
        DBJ_MAKE_HASHMAP(short, bool)           // the map

    K and V must each be a SINGLE token, because the generated names are
    pasted out of them: `unsigned int` needs a typedef first, `short` and
    `dbj_str_slice` are fine.

    K must answer two operations, by name, before the map is generated:

        uint64_t K_hash (K);
        bool     K_equal(K, K);

    There is no default for either, and no shorthand for "small type, use
    the value". No type is its own hash. The bits of a short are not a
    hash of that short, they are the short; what makes a key a key is the
    hash function chosen for it, and choosing it is the caller's job every
    time. The shorthand would also be a trap the moment K is a double: a
    cast to uint64_t truncates 0.5 to 0, and every value below 1 becomes
    the same key.

    They are named, not stored. A hash function pointer inside every key
    would cost eight bytes in every slot and an indirect call on every
    probe step -- the macro already knows the key type, so it pastes the
    call instead and the optimiser folds it.

    Configure before including, or take the default:

        DBJ_HASHMAP_SLOTS   slot count, a power of two (1024), shared by
                            every map generated in the translation unit
*/
#include <dbj_required_compile_time.h>

#include <dbj_result.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h> /* snprintf, for DBJ_MAKERESULT's err factory */

#ifndef DBJ_HASHMAP_SLOTS
#define DBJ_HASHMAP_SLOTS 1024
#endif

#define DBJ_HASHMAP_MASK (DBJ_HASHMAP_SLOTS - 1)

static_assert((DBJ_HASHMAP_SLOTS & DBJ_HASHMAP_MASK) == 0,
              "DBJ_HASHMAP_SLOTS must be a power of two -- the probe relies on it");

/* ------------------------------------------------------------------
   Shared by every generated map -- written once, not once per
   instantiation. Nothing here depends on K or V.
   ------------------------------------------------------------------ */

/* EMPTY  nothing was ever stored here; key and value mean nothing
   NULL   a key is present, holding no value -- the SQL NULL of this map
   VALUE  a key with a value

   The state belongs to the slot, not to the key and not to the value.
   A zeroed map is an empty map, because DBJ_SLOT_EMPTY is zero. */
typedef enum
{
    DBJ_SLOT_EMPTY,
    DBJ_SLOT_NULL,
    DBJ_SLOT_VALUE
} dbj_slot_state;

/* One entry of the index: the slot's state and the hash that put the key
   there. Sixteen bytes, four of them padding after the enum. */
typedef struct
{
    dbj_slot_state state;
    uint64_t hash;
} dbj_slot_index;

/* index and hash mean something only when found is true. hash is carried
   out because a set must write it into the slot, and rehashing the key to
   do that would throw away the work the probe just did. */
typedef struct
{
    bool found;
    uint32_t index; /* uint32_t, not ptrdiff_t, to hold this to 16 bytes:
                       a 24-byte struct returns through memory, a 16-byte
                       one of integer fields returns in rax:rdx */
    uint64_t hash;
} dbj_hashmap_probe;

/* splitmix64's finaliser. Consecutive ordinal keys (0, 1, 2, ...) land in
   consecutive slots -- fine for the index, but it leaves the probe step
   correlated. This makes the high bits, where the step comes from, as good
   as the low ones. A key that arrives already hashed loses nothing by being
   mixed again.

   [[gnu::const]]: the result depends on the arguments and nothing else, so
   repeated calls fold into one. */
[[gnu::const]] static inline uint64_t dbj_hashmap_mix64(uint64_t key, uint64_t seed)
{
    uint64_t hash = key + seed;
    hash ^= hash >> 30;
    hash *= 0xbf58476d1ce4e5b9u;
    hash ^= hash >> 27;
    hash *= 0x94d049bb133111ebu;
    hash ^= hash >> 31;
    return hash;
}

/* ------------------------------------------------------------------
   Reading a result

   The OK arm's member name is built from the element type, which is the
   one thing these cannot guess -- hence the first argument. Spell it
   DBJ_HASHMAP_ELEMENT_TYPE(K, V) rather than by hand.
   ------------------------------------------------------------------ */

#define DBJ_HASHMAP_TYPE(K_, V_) hashmap_##K_##_##V_
#define DBJ_HASHMAP_ELEMENT_TYPE(K_, V_) hashmap_##K_##_##V_##_element

/* Valid only once dbj_result_is_ok() has said so. C cannot enforce that.

   Note what dbj_result_is_ok does NOT say: whether the key was found. A
   missing key is a successful lookup. */
#define dbj_hashmap_element(E_, result_) ((result_).E_##_OK.my_value)

/* DBJ_SLOT_EMPTY, _NULL or _VALUE -- the usual question after a get. */
#define dbj_hashmap_state(E_, result_) (dbj_hashmap_element(E_, result_).state)

/* Meaningful only when the state is DBJ_SLOT_VALUE. */
#define dbj_hashmap_value(E_, result_) (dbj_hashmap_element(E_, result_).val)

#define dbj_hashmap_key(E_, result_) (dbj_hashmap_element(E_, result_).key)

/* Empty the map. There is no constructor: all-zero is the empty map, which
   is why a static map needs no call at all. This is for reusing one that
   has been filled. Takes a pointer, and needs no type argument. */
#define dbj_hashmap_clear(map_) memset((map_), 0, sizeof *(map_))

/* ------------------------------------------------------------------
   The map itself
   ------------------------------------------------------------------ */

/*  Generates, for DBJ_MAKE_HASHMAP(short, bool):

        hashmap_short_bool_element          state + key + val
        hashmap_short_bool_elementResult    and its factories
        hashmap_short_bool                  the map
        hashmap_short_bool_get / _set / _set_null / _count

    Three arrays, not one array of elements. The probe reads the state and
    the hash and nothing else, so an array of whole elements would drag a
    value into L1 at every step to look at four bytes of tag. Split, the
    probe walks the index only -- 16 KB at the default slot count, which
    stays in L1 whatever the value type -- and touches a key only when the
    stored hash already matched. Total memory is unchanged; what changed is
    which of it the probe has to walk.

    always_inline on the operations is measured and not a preference.
    Assembling an element out of three arrays costs inliner budget, and
    past the threshold GCC puts _set out of line -- at which point a large
    V has to be copied onto the stack at every call site, an expense that
    does not exist when the call is inlined. */
#define DBJ_MAKE_HASHMAP(K_, V_)                                                       \
    typedef struct                                                                     \
    {                                                                                  \
        dbj_slot_state state;                                                          \
        K_ key;                                                                        \
        V_ val; /* meaningful only when state == DBJ_SLOT_VALUE */                     \
    } hashmap_##K_##_##V_##_element;                                                   \
                                                                                       \
    DBJ_MAKERESULT(hashmap_##K_##_##V_##_element);                                     \
                                                                                       \
    typedef struct                                                                     \
    {                                                                                  \
        dbj_slot_index index[DBJ_HASHMAP_SLOTS];                                       \
        K_ keys[DBJ_HASHMAP_SLOTS];                                                    \
        V_ vals[DBJ_HASHMAP_SLOTS];                                                    \
    } hashmap_##K_##_##V_;                                                             \
                                                                                       \
    /* The three arrays reassembled into the element the caller sees. The               \
       element is the shape of an answer, not the shape of storage. */                  \
    [[nodiscard, gnu::always_inline, maybe_unused]] static inline                       \
        hashmap_##K_##_##V_##_element                                                   \
        hashmap_##K_##_##V_##_element_at(const hashmap_##K_##_##V_ *map_,               \
                                         ptrdiff_t at_)                                 \
    {                                                                                  \
        return (hashmap_##K_##_##V_##_element){.state = map_->index[at_].state,         \
                                               .key = map_->keys[at_],                  \
                                               .val = map_->vals[at_]};                 \
    }                                                                                  \
                                                                                       \
    /* The slot already holding key_, or the first free one to claim.                   \
                                                                                       \
       Double hashing: low bits index, high bits supply an odd step, which               \
       is coprime with a power-of-two size and so visits every slot exactly              \
       once. That is why the loop is bounded by the slot count -- it is the              \
       real termination condition. Without it a lookup in a *full* table                 \
       (no match, no free slot) never returns, and that hits reads too.                  \
                                                                                       \
       The map's own address seeds the mix, so ASLR randomises it and no                 \
       seed is stored.                                                                   \
                                                                                       \
       DBJ_SLOT_NULL does not stop the search: a null-valued entry is a                  \
       present key, so probing continues past it as it would past a value.               \
                                                                                       \
       The stored hash is compared first. It cannot prove two keys equal --              \
       a collision is still a collision -- so K_##_equal decides, but it now             \
       runs only on slots whose hash already matched. */                                 \
    [[nodiscard, maybe_unused]] static inline dbj_hashmap_probe                          \
        hashmap_##K_##_##V_##_slot(const hashmap_##K_##_##V_ *map_, K_ key_)             \
    {                                                                                  \
        uint64_t hash_ = dbj_hashmap_mix64(K_##_hash(key_), (uintptr_t)map_);            \
        uint32_t step_ = (uint32_t)(hash_ >> 32) | 1;                                    \
        uint32_t at_ = (uint32_t)hash_;                                                  \
                                                                                       \
        for (int probes_ = 0; probes_ < DBJ_HASHMAP_SLOTS; probes_++)                    \
        {                                                                              \
            at_ = (at_ + step_) & DBJ_HASHMAP_MASK;                                      \
            const dbj_slot_index *slot_ = &map_->index[at_];                             \
                                                                                       \
            switch (slot_->state)                                                        \
            {                                                                          \
            case DBJ_SLOT_EMPTY:                                                         \
                /* never written: the key is not here, and here is where it               \
                   would go */                                                            \
                return (dbj_hashmap_probe){.found = true, .index = at_, .hash = hash_};  \
            case DBJ_SLOT_NULL:                                                          \
            case DBJ_SLOT_VALUE:                                                         \
                if (slot_->hash == hash_ && K_##_equal(map_->keys[at_], key_))            \
                {                                                                      \
                    return (dbj_hashmap_probe){                                          \
                        .found = true, .index = at_, .hash = hash_};                     \
                }                                                                      \
                break;                                                                   \
            }                                                                          \
        }                                                                              \
        return (dbj_hashmap_probe){0}; /* every slot visited: the table is full */       \
    }                                                                                  \
                                                                                       \
    /* The element comes back as a copy, so the caller holds no pointer into            \
       the map. A key that is not present is not a failure: OK, with state              \
       DBJ_SLOT_EMPTY. ERR means one thing -- the map could not answer.                 \
                                                                                       \
       [[nodiscard]] throughout: the result is the only report that the map              \
       was full, so dropping it must be spelled (void). */                               \
    [[nodiscard, gnu::always_inline, maybe_unused]] static inline                        \
        hashmap_##K_##_##V_##_elementResult                                              \
        hashmap_##K_##_##V_##_get(const hashmap_##K_##_##V_ *map_, K_ key_)              \
    {                                                                                  \
        dbj_hashmap_probe probe_ = hashmap_##K_##_##V_##_slot(map_, key_);               \
        if (!probe_.found)                                                               \
        {                                                                              \
            return hashmap_##K_##_##V_##_element_make_err(__func__,                      \
                                                          "hashmap capacity exhausted"); \
        }                                                                              \
        return hashmap_##K_##_##V_##_element_make_ok(                                    \
            hashmap_##K_##_##V_##_element_at(map_, probe_.index));                       \
    }                                                                                  \
                                                                                       \
    /* Insert, or replace the value of a key already present. */                        \
    [[nodiscard, gnu::always_inline, maybe_unused]] static inline                        \
        hashmap_##K_##_##V_##_elementResult                                              \
        hashmap_##K_##_##V_##_set(hashmap_##K_##_##V_ *map_, K_ key_, V_ val_)           \
    {                                                                                  \
        dbj_hashmap_probe probe_ = hashmap_##K_##_##V_##_slot(map_, key_);               \
        if (!probe_.found)                                                               \
        {                                                                              \
            return hashmap_##K_##_##V_##_element_make_err(__func__,                      \
                                                          "hashmap capacity exhausted"); \
        }                                                                              \
        map_->index[probe_.index] =                                                      \
            (dbj_slot_index){.state = DBJ_SLOT_VALUE, .hash = probe_.hash};              \
        map_->keys[probe_.index] = key_;                                                 \
        map_->vals[probe_.index] = val_;                                                 \
        return hashmap_##K_##_##V_##_element_make_ok(                                    \
            hashmap_##K_##_##V_##_element_at(map_, probe_.index));                       \
    }                                                                                  \
                                                                                       \
    /* The key stays present, so probing does not stop at it and a later get             \
       reports DBJ_SLOT_NULL, not DBJ_SLOT_EMPTY. */                                     \
    [[nodiscard, gnu::always_inline, maybe_unused]] static inline                        \
        hashmap_##K_##_##V_##_elementResult                                              \
        hashmap_##K_##_##V_##_set_null(hashmap_##K_##_##V_ *map_, K_ key_)               \
    {                                                                                  \
        dbj_hashmap_probe probe_ = hashmap_##K_##_##V_##_slot(map_, key_);               \
        if (!probe_.found)                                                               \
        {                                                                              \
            return hashmap_##K_##_##V_##_element_make_err(__func__,                      \
                                                          "hashmap capacity exhausted"); \
        }                                                                              \
        map_->index[probe_.index] =                                                      \
            (dbj_slot_index){.state = DBJ_SLOT_NULL, .hash = probe_.hash};               \
        map_->keys[probe_.index] = key_;                                                 \
        map_->vals[probe_.index] = (V_){0};                                              \
        return hashmap_##K_##_##V_##_element_make_ok(                                    \
            hashmap_##K_##_##V_##_element_at(map_, probe_.index));                       \
    }                                                                                  \
                                                                                       \
    /* Slots holding a key, DBJ_SLOT_VALUE and DBJ_SLOT_NULL alike. */                  \
    [[nodiscard, maybe_unused]] static inline ptrdiff_t                                  \
        hashmap_##K_##_##V_##_count(const hashmap_##K_##_##V_ *map_)                     \
    {                                                                                  \
        ptrdiff_t used_ = 0;                                                             \
        for (ptrdiff_t i = 0; i < DBJ_HASHMAP_SLOTS; i++)                                \
        {                                                                              \
            used_ += map_->index[i].state != DBJ_SLOT_EMPTY;                             \
        }                                                                              \
        return used_;                                                                    \
    }
