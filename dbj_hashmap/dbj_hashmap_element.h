#pragma once
/*
    2026AUG29       (c) dbj@dbj.org

    dbj_hashmap_element.h -- one slot: a key handle and a value.

    The only file that knows about both sides. The key handle's id says
    whether the value means anything, so the two are never read
    independently.
*/
#include <dbj_required_compile_time.h>

#include "dbj_hash_key.h"
#include "dbj_hash_string.h"

typedef struct
{
    HashKeyHandle key;
    HashString val;
} HashMapElement;

/* HashMapElementResult, which wraps this type, lives next door in
   dbj_hashmap_element_result.h -- the stored types are one concern, the
   result wrapper another. */
