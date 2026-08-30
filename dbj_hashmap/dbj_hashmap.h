#pragma once
/*
    2026AUG30       (c) dbj@dbj.org

    dbj_hashmap.h -- the union-keyed map, as one instantiation of
    DBJ_MAKE_HASHMAP.

        #define DBJ_MAKERESULT_IMPLEMENTATION   // in exactly one .c
        #include <dbj_hashmap.h>

    This whole file is now three includes and one line of expansion. The
    map machinery -- probe, index, states, result -- lives in
    dbj_make_hashmap.h and belongs to no particular key or value type.
    What is left here is the choice of the two types:

        HashKey     a discriminated union over key kinds, from
                    dbj_hash_key.h. It answers HashKey_hash and
                    HashKey_equal, which is all the map ever asks of a
                    key.
        HashString  a discriminated union over dbj_str sizes, from
                    dbj_hash_string.h.

    Both are ordinary user types. A map whose key is a plain `short` is
    the same macro with a different pair of arguments, and costs three
    bytes per slot where this one costs a hundred and eighty -- that is
    the tag being paid for, and here it is paid on purpose: one map type
    that holds any key kind and any string size.

    Generated names follow the two arguments, so the operations are
    hashmap_HashKey_HashString_get and its three siblings.

    Configure before including, or take the defaults:

        KeyOrdinal          the ordinal key type (unsigned int)
        KeyString           the string key type (str32)
        DBJ_HASHMAP_SLOTS   slot count, a power of two (1024)
*/
#include <dbj_required_compile_time.h>

#include "dbj_arena.h"
#include "dbj_hash_key.h"
#include "dbj_hash_string.h"
#include "dbj_make_hashmap.h"

DBJ_MAKE_HASHMAP(HashKey, HashString)
