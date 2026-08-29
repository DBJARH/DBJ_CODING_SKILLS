#pragma once
/*
    2026AUG28       (c) dbj@dbj.org

    dbj_hash_string.h -- the value, key and element types for a
    fixed-capacity hash map, all as discriminated unions.

    A library header, not demo code: nothing here knows about a
    particular map, a particular size, or a particular application.
    dbj-str-4-welons.c is one user of it.

    Three types, in the order they compose:

        HashString      the value. Any one dbj_str size, discriminated
                        by type_id. A pure value type -- it carries no
                        notion of being present or absent.

        HashKey         the key, and with it the state of the slot:
                        EMPTY, NULL or VALUE, the same three states a
                        single SQL cell has. All slot behaviour lives
                        here.

        HashMapElement  one slot: a key and a value.

    Nothing points anywhere. Every one of these is a value, copied in
    and out, so no lifetime question arises and no arena has to outlive
    anything the caller keeps.
*/
#include <dbj_required_compile_time.h>

#include <dbj_result.h>
#include <dbj_str.h>

#include <stddef.h>
#include <stdio.h>  /* snprintf, for DBJ_MAKERESULT's err factory */
#include <string.h>

/* The dbj_str sizes a HashString can hold. The union is as large as
   its largest member, so adding a size costs every HashString value.
   Extending the list means extending HashStringTypeID and every switch
   over it -- which is the point: no `default` case anywhere, so
   -Wswitch -Werror names each site that has to be updated. */
DEFINE_DBJSTR_TYPE(str32, 32)
DEFINE_DBJSTR_TYPE(str64, 64)
DEFINE_DBJSTR_TYPE(str128, 128)

typedef enum
{
    DBJSTR32,
    DBJSTR64,
    DBJSTR128
} HashStringTypeID;

/* The value type. type_id says which size is live -- nothing more. A
   HashString is never "empty" or "null"; that is the key's business. */
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

/* Content length. Capacity is sizeof(s.data); the content stops at the
   first NUL, or at the end of the buffer when it is completely full. */
#define dbj_str_len(text_) \
    (strnlen((const char *)(text_).data, sizeof((text_).data)))

/* Pad a C string out to a full buffer of the given dbj_str type,
   truncating what does not fit. All-zero is a valid empty value, so no
   constructor is needed.

   One filler per size rather than one macro over all of them: a macro
   able to declare a local of the parameter type needs a statement
   expression, which is a GNU extension, and these are three short
   functions. */
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

/* Read the text out, whichever size is live. Returns a pointer into
   the caller's own HashString -- valid exactly as long as that value
   is, which is why HashString is always passed by pointer here and by
   value everywhere else.

   No `default` case: adding a HashStringTypeID must break this build. */
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

/* ------------------------------------------------------------------
   HashKey -- the key, and the state of the slot holding it
   ------------------------------------------------------------------ */

/* The key type is ordinal, not text: it is its own hash, so there is
   no string to store and none to compare. An application redefines
   this before including if it wants something else. */
#ifndef KeyType
#define KeyType unsigned int
#endif

/* Three states:

       EMPTY   nothing was ever stored here
       NULL    a key is present, but it holds no value
       VALUE   a key with a value

   EMPTY and NULL carry no data. C has no zero-sized type, so each gets
   a placeholder byte; the union is sized by ValidKey regardless. */
typedef struct
{
    char unused;
} EmptyKey;

typedef struct
{
    char unused;
} NullKey;

typedef struct
{
    KeyType key;
} ValidKey;

typedef enum
{
    HK_EMPTY,
    HK_NULL,
    HK_VALUE
} hk_id;

typedef struct
{
    hk_id id;
    union
    {
        EmptyKey empty;
        NullKey null;
        ValidKey val;
    };
} HashKey;

/* Factories. A zeroed HashKey is HK_EMPTY, which is what makes a
   zeroed map an empty map -- so hash_key_empty() exists for clarity at
   call sites, not because anything needs it to initialise. That is
   also why it is [[maybe_unused]]: nothing in this library calls it,
   and that is correct rather than an oversight. */
[[nodiscard, maybe_unused]] static inline HashKey hash_key_empty(void)
{
    return (HashKey){.id = HK_EMPTY, .empty = {0}};
}

/* HK_NULL still remembers which key it is: the entry is present, so a
   probe must be able to match it. Only HK_EMPTY has no key at all,
   which is why NullKey is unused in practice -- kept so the union
   states every case it can be in. */
[[nodiscard]] static inline HashKey hash_key_null(KeyType key)
{
    return (HashKey){.id = HK_NULL, .val = {.key = key}};
}

[[nodiscard]] static inline HashKey hash_key_value(KeyType key)
{
    return (HashKey){.id = HK_VALUE, .val = {.key = key}};
}

/* ------------------------------------------------------------------
   HashMapElement -- one slot in a hash/map
   ------------------------------------------------------------------ */

/* A key and its value. The key's id says whether the value means
   anything: on HK_EMPTY it does not, and on HK_NULL it is explicitly
   absent. */
typedef struct
{
    HashKey key;
    HashString val;
} HashMapElement;

/* HashMapElementResult: OK carries an element, ERR carries a location
   and a message. See toplevel/dbj_result.h.

   Note what is *not* an error: a key that is not in the map comes back
   OK, holding an element whose key.id is HK_EMPTY. "Absent" is an
   answer, not a failure. ERR is reserved for the map being unable to
   answer at all -- capacity exhausted. */
DBJ_MAKERESULT(HashMapElement);
