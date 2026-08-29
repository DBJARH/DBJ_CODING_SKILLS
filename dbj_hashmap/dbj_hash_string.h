#pragma once
/*
    2026AUG28       (c) dbj@dbj.org

    dbj_hash_string.h -- the value type, a discriminated union.
    Nothing here knows about a particular map.

        HashString      the value: any one dbj_str size, tagged by
                        type_id. Never "empty" or "null" -- that is
                        the key's business, and it lives next door in
                        dbj_hash_key.h.

    Nothing points anywhere. A HashString is a value, copied in and
    out, so no lifetime question arises.
*/
#include <dbj_required_compile_time.h>

#include "dbj_hash_key.h"

#include <dbj_result.h>
#include <dbj_str.h>

#include <stddef.h>
#include <stdio.h>  /* snprintf, for DBJ_MAKERESULT's err factory */
#include <string.h>

/* The union is as large as its largest member, so adding a size costs
   every HashString value. No `default` case anywhere below, so
   -Wswitch-enum -Werror names each site an added size must update. */
/* str32 is declared by dbj_hash_key.h -- KeyString defaults to it, so
   the key side cannot wait for this one. These two are the sizes only
   the value side uses. */
DEFINE_DBJSTR_TYPE(str64, 64)
DEFINE_DBJSTR_TYPE(str128, 128)

typedef enum
{
    DBJSTR32,
    DBJSTR64,
    DBJSTR128
} HashStringTypeID;

/* type_id says which size is live -- nothing more. */
typedef struct
{
    HashStringTypeID type_id;
    union
    {
        str32 val32;
        str64 val64;
        str128 val128;
    };
} HashString;

/* ------------------------------------------------------------------
   HashString factories and accessors
   ------------------------------------------------------------------ */

/* Content stops at the first NUL, or at the end of a full buffer.
   Capacity is sizeof(s.data). */
#define dbj_str_len(text_) \
    (strnlen((const char *)(text_).data, sizeof((text_).data)))

/* Pad a C string out to the full buffer, truncating what does not fit.

   One filler per size, not one macro over all: declaring a local of a
   macro parameter's type needs a statement expression, a GNU
   extension, and these are three short functions. */
#define DBJ_HASH_STRING_FILLER(type_)                      \
    [[nodiscard]] static inline type_ type_##_fill(const char *text) \
    {                                                      \
        type_ result = {0};                                \
        size_t length = strlen(text);                      \
        if (length > sizeof(result.data))                  \
        {                                                  \
            length = sizeof(result.data); /* truncate */   \
        }                                                  \
        memcpy(result.data, text, length);                 \
        return result;                                     \
    }

DBJ_HASH_STRING_FILLER(str32)
DBJ_HASH_STRING_FILLER(str64)
DBJ_HASH_STRING_FILLER(str128)

[[nodiscard]] static inline HashString hash_string_32(const char *text)
{
    return (HashString){.type_id = DBJSTR32, .val32 = str32_fill(text)};
}

[[nodiscard]] static inline HashString hash_string_64(const char *text)
{
    return (HashString){.type_id = DBJSTR64, .val64 = str64_fill(text)};
}

[[nodiscard, maybe_unused]] static inline HashString hash_string_128(const char *text)
{
    return (HashString){.type_id = DBJSTR128, .val128 = str128_fill(text)};
}

/* Returns a pointer into the caller's own HashString, valid exactly as
   long as that value is -- which is why this one takes a pointer while
   everything else takes a value. */
[[nodiscard]] static inline const char *hash_string_text(const HashString *text)
{
    switch (text->type_id)
    {
    case DBJSTR32:
        return (const char *)text->val32.data;
    case DBJSTR64:
        return (const char *)text->val64.data;
    case DBJSTR128:
        return (const char *)text->val128.data;
    }
    __builtin_unreachable(); /* no fake return; the switch has no default */
}

/* Content length of whichever size is live. */
[[nodiscard, maybe_unused]] static inline size_t hash_string_len(const HashString *text)
{
    switch (text->type_id)
    {
    case DBJSTR32:
        return dbj_str_len(text->val32);
    case DBJSTR64:
        return dbj_str_len(text->val64);
    case DBJSTR128:
        return dbj_str_len(text->val128);
    }
    __builtin_unreachable(); /* no fake return; the switch has no default */
}
