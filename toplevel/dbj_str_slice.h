#pragma once
/*
    2026AUG28       (c) dbj@dbj.org

    dbj_str_slice -- a borrowed string: pointer + length, never
    null-terminated, owning nothing.

    Adapted from Chris Wellons' `Str`:
        https://nullprogram.com/blog/2025/01/19/
    (public domain / Unlicense upstream)

    This is the *complement* of DEFINE_DBJSTR_TYPE in dbj_str.h, not a
    replacement for it. The two answer different questions:

        dbj_str.h    str256, str1024, ...   owned, fixed-size, by value,
                                            no length field -- the whole
                                            buffer travels with the value

        this header  dbj_str_slice          borrowed, 16 bytes, carries a
                                            length -- points into a
                                            literal, an arena, a mapped
                                            file, or nowhere at all

    Which of the two is the better default for this repo is an open
    question -- deliberately so. See chris_welons/readme.md.

    All-bits-zero is a valid empty slice, so no constructor is needed.
    That zero state is exactly why the null-pointer guards below exist:
    memcmp and memcpy forbid null pointers even for a zero count.
*/
#include <dbj_required_compile_time.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct
{
    char *data;
    ptrdiff_t len;
} dbj_str_slice;

/* String literal -> dbj_str_slice, length computed at compile time,
   no strlen. Only ever pass a literal or a true char array. */
#define DBJ_SS(s_) (dbj_str_slice){(s_), sizeof(s_) - 1}

/* Byte equality. The !left.len short-circuit is not redundant: memcmp
   arbitrarily forbids null pointers, and either side may be zero. */
static inline bool dbj_str_slice_equals(dbj_str_slice left, dbj_str_slice right)
{
    if (left.len != right.len)
    {
        return false;
    }
    return !left.len || !memcmp(left.data, right.data, left.len);
}

/* FNV-style multiplicative hash, seedable.

   The seed keeps an attacker who controls the keys from manufacturing
   collisions -- callers in this repo pass a live address (ASLR does
   the randomising) rather than storing a seed anywhere. Pass 0 for the
   plain unseeded hash.

   Masking with 255 keeps the result independent of whether char is
   signed. The high bits are the well-mixed ones, so consumers should
   take their bits from the top. */
static inline uint64_t dbj_str_slice_hash(dbj_str_slice text, uint64_t seed)
{
    uint64_t hash = seed ? seed : 0x100;
    for (ptrdiff_t i = 0; i < text.len; i++)
    {
        hash ^= text.data[i] & 255;
        hash *= 1111111111111111111u;
    }
    return hash;
}
