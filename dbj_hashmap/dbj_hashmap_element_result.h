#pragma once
/*
    2026AUG29       (c) dbj@dbj.org

    dbj_hashmap_element_result.h -- HashMapElementResult and its accessors.

        #define DBJ_MAKERESULT_IMPLEMENTATION   // in exactly one .c
        #include "dbj_hashmap_element_result.h"

    That #define comes from toplevel/dbj_result.h, whose factories are
    ordinary functions and so belong to a single translation unit.
    Everything this header adds is a macro.

    HashMapElementResult is what every dbj_hashmap operation returns:

        tag == DBJ_RESULT_OK    HashMapElement_OK.my_value is the element
        tag == DBJ_RESULT_ERR   HashMapElement_ERR.location / .message

    A key that is simply not in the map is OK, carrying an element whose
    key.id is HK_EMPTY. Absence is an answer. ERR is reserved for the
    map being unable to answer at all -- capacity exhausted.
*/
#include <dbj_required_compile_time.h>

#include "dbj_hash_string.h"

#include <stdio.h> /* snprintf, for DBJ_MAKERESULT's err factory */

DBJ_MAKERESULT(HashMapElement);

/* Did the call succeed? Says nothing about whether the key was found --
   a missing key is a successful lookup. */
#define dbj_result_is_ok(result_) ((result_).tag == DBJ_RESULT_OK)

#define dbj_result_is_err(result_) ((result_).tag == DBJ_RESULT_ERR)

/* The element out of the OK arm.

   Reading the OK arm of an ERR result reads the wrong union member, so
   these three are valid only once dbj_result_is_ok() has said so. That
   is the contract; C cannot enforce it here. */
#define dbj_result_element(result_) ((result_).HashMapElement_OK.my_value)

/* The slot state carried by an OK result: HK_EMPTY, HK_NULL or
   HK_VALUE. This is the usual question after a get -- was the key
   there, and did it hold anything. */
#define dbj_result_state(result_) (dbj_result_element(result_).key.id)

/* The value out of an OK result. Meaningful when the state is
   HK_VALUE; on HK_EMPTY and HK_NULL there is no value to speak of. */
#define dbj_result_value(result_) (dbj_result_element(result_).val)

/* The two halves of the ERR arm -- valid only once dbj_result_is_err()
   has said so, for the same reason as the OK accessors above. Both are
   fixed-size char arrays, so they are safe to print with %s. */
#define dbj_result_location(result_) ((result_).HashMapElement_ERR.location)

#define dbj_result_message(result_) ((result_).HashMapElement_ERR.message)
