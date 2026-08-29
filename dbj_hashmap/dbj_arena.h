#pragma once
/*
    2026AUG29       (c) dbj@dbj.org

    dbj_arena.h -- bump allocator. Header only.

        #include <dbj_arena.h>

    No DBJ_ARENA_IMPLEMENTATION switch: every function here is small
    enough to be `static inline`, so there is no object code to place
    in one translation unit and nothing for a second one to duplicate.
    The STB split exists to solve a problem this header does not have.

    The whole allocator is two pointers: `beg` is the bump cursor,
    `end` the hard limit. There is no free, and that is the design
    rather than an omission -- passing a dbj_arena *by value* hands the
    callee a scratch copy, and everything it allocated is gone the
    moment it returns.

    Nothing here is map-specific; dbj_hashmap.h happens to be one user.
*/
#include <dbj_required_compile_time.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct
{
    char *beg;
    char *end;
} dbj_arena;

/* Factory: wrap an already-owned block. The caller keeps ownership;
   the arena only ever points into it.

   Evaluates block_ twice, so pass a variable, not a call. */
#define dbj_arena_make(block_, size_) \
    ((dbj_arena){(block_), (block_) + (size_)})

/* Allocate count objects of type_, zeroed and correctly aligned.

   The cast turns a type mismatch into a diagnostic, and sizeof and
   _Alignof are taken from the type, so a caller cannot get the
   arithmetic wrong. Evaluates to nullptr when the arena cannot
   satisfy the request. */
#define dbj_arena_new(arena_, count_, type_) \
    (type_ *)dbj_arena_alloc((arena_), (count_), sizeof(type_), _Alignof(type_))

/* Bump `count` objects of `size` bytes off the front, aligned and
   zeroed. Zeroed is what lets a structure be valid at all-bits-zero,
   so nothing allocated here needs a constructor.

   Returns nullptr rather than asserting when the block cannot satisfy
   the request: exhaustion is an answer the caller can act on, not a
   defect to abort over. Every caller must check. */
static inline void *dbj_arena_alloc(dbj_arena *arena, ptrdiff_t count, ptrdiff_t size, ptrdiff_t align)
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
