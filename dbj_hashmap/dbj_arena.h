#pragma once
/*
    2026AUG29       (c) dbj@dbj.org

    dbj_arena.h -- bump allocator.

        #define DBJ_MAKERESULT_IMPLEMENTATION   // in exactly one .c
        #include "dbj_arena.h"

    Two pointers: `beg` is the bump cursor, `end` the hard limit. There
    is no free -- passing a dbj_arena *by value* hands the callee a
    scratch copy, and everything it allocated is gone when it returns.

    Running out is an ordinary outcome, not a defect, so it is reported
    rather than asserted: dbj_arena_alloc returns a result carrying
    either the block or the reason there is none. See "Exhaustion is an
    answer" below.

    Nothing here is map-specific; dbj_hashmap.h is one user.
*/
#include <dbj_required_compile_time.h>

#include <dbj_result.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct
{
    char *beg;
    char *end;
} dbj_arena;

/* The caller keeps the block; the arena only points into it.
   Evaluates block_ twice, so pass a variable, not a call. */
#define dbj_arena_make(block_, size_) \
    ((dbj_arena){(block_), (block_) + (size_)})

/* ------------------------------------------------------------------
   Exhaustion is an answer

   dbj_arena_alloc used to return nullptr, and that was not good
   enough. A bare nullptr says only "no", never why, and the caller
   above it -- hash_key_slice_copy was the real case -- is then tempted
   to swallow it and hand back something that looks valid. Which is the
   magical-constant failure this repo spends its time avoiding.

   So a result comes back instead: either the block, or a location and
   a message. Running out of arena is a thing programs are expected to
   survive, and to survive it the caller has to be told.

   void * cannot be pasted into an identifier, so it is given a name
   first -- DBJ_MAKERESULT builds dbj_arena_blockResult out of it.
   ------------------------------------------------------------------ */

typedef void *dbj_arena_block;

DBJ_MAKERESULT(dbj_arena_block);

/* What every dbj_arena call that can fail returns.

   DBJ_MAKERESULT names its type after the payload, so it made
   dbj_arena_blockResult. That name answers "what is inside", which is
   not the question a caller asks -- the question is "what does this
   API hand me", and the answer is one type for the whole of it. So
   dbj_arena_block is an implementation detail of the generation from
   here on, and the accessors below keep it off call sites. */
typedef dbj_arena_blockResult dbj_arena_result;

/* The arena's result subtypes. One failure, so far -- the enum exists
   so that adding a second is a value here rather than a new string
   somewhere, and so a caller switches on a tag instead of reading
   prose.

   unsigned short to match the result's code field exactly. NONE = 0 is
   the generic "the maker did not say", spelled in this enum's own
   terms so a switch over it is exhaustive. */
typedef enum : unsigned short
{
    DBJ_ARENA_ERR_NONE = 0,
    DBJ_ARENA_ERR_EXHAUSTED,
} dbj_arena_result_type;

/* The subtype of an arena failure. Meaningful only after
   dbj_result_is_err(). */
#define dbj_arena_result_type_of(result_) \
    ((dbj_arena_result_type)dbj_result_code(dbj_arena_block, (result_)))

/* The two human-readable ERR fields, without the caller having to
   name dbj_arena_block. Meaningful only after dbj_result_is_err(). */
#define dbj_arena_result_location(result_) \
    dbj_result_location(dbj_arena_block, (result_))

#define dbj_arena_result_message(result_) \
    dbj_result_message(dbj_arena_block, (result_))

/* The OK arm. Valid only once dbj_result_is_ok() has said so. */
#define dbj_result_block(result_) ((result_).dbj_arena_block_OK.my_value)

/* sizeof/_Alignof come from the type, so the caller cannot get the
   arithmetic wrong. Returns the result, not a pointer -- check it,
   then take the pointer out with dbj_arena_ptr below. */
#define dbj_arena_new(arena_, count_, type_) \
    dbj_arena_alloc((arena_), (count_), sizeof(type_), _Alignof(type_))

/* The block, typed. The cast makes a type mismatch a diagnostic; it
   is separate from dbj_arena_new because a result is what has to be
   checked and a pointer is what cannot be. */
#define dbj_arena_ptr(result_, type_) \
    ((type_ *)dbj_result_block(result_))

/* The block comes back zeroed, which is what lets a structure be valid
   at all-bits-zero. */
[[nodiscard]] static inline dbj_arena_result dbj_arena_alloc(dbj_arena *arena, ptrdiff_t count, ptrdiff_t size, ptrdiff_t align)
{
    /* bytes needed to round beg up to `align` -- align is a power of two */
    ptrdiff_t pad = -(uintptr_t)arena->beg & (align - 1);
    /* division, not multiplication, so the check itself cannot overflow */
    if (count >= (arena->end - arena->beg - pad) / size)
    {
        return dbj_arena_block_make_err_coded(
            __func__, "arena exhausted: the request does not fit in what is left",
            DBJ_ARENA_ERR_EXHAUSTED);
    }
    void *result = arena->beg + pad;
    arena->beg += pad + count * size;
    return dbj_arena_block_make_ok(memset(result, 0, count * size));
}

/* Bytes still available, ignoring alignment padding. The caller that
   has just been told "exhausted" usually wants this next, and it is
   cheaper to expose than to carry in the message. */
[[nodiscard, maybe_unused]] static inline ptrdiff_t dbj_arena_left(const dbj_arena *arena)
{
    return arena->end - arena->beg;
}
