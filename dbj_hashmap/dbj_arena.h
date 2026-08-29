#pragma once
/*
    2026AUG29       (c) dbj@dbj.org

    dbj_arena.h -- bump allocator. Header only, all static inline, so
    there is no implementation switch to define.

    Two pointers: `beg` is the bump cursor, `end` the hard limit. There
    is no free -- passing a dbj_arena *by value* hands the callee a
    scratch copy, and everything it allocated is gone when it returns.

    Nothing here is map-specific; dbj_hashmap.h is one user.
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

/* The caller keeps the block; the arena only points into it.
   Evaluates block_ twice, so pass a variable, not a call. */
#define dbj_arena_make(block_, size_) \
    ((dbj_arena){(block_), (block_) + (size_)})

/* The cast makes a type mismatch a diagnostic; sizeof/_Alignof come
   from the type, so the caller cannot get the arithmetic wrong.
   nullptr when the arena cannot satisfy the request. */
#define dbj_arena_new(arena_, count_, type_) \
    (type_ *)dbj_arena_alloc((arena_), (count_), sizeof(type_), _Alignof(type_))

/* Zeroed, which is what lets a structure be valid at all-bits-zero.

   nullptr rather than an assert on exhaustion: that is an answer the
   caller can act on, not a defect to abort over. Callers must check. */
[[nodiscard]] static inline void *dbj_arena_alloc(dbj_arena *arena, ptrdiff_t count, ptrdiff_t size, ptrdiff_t align)
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
