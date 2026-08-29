#pragma once
/*
    2026AUG29       (c) dbj@dbj.org

    dbj_hashmap_element_result.h -- HashMapElementResult and its accessors.

        #define DBJ_MAKERESULT_IMPLEMENTATION   // in exactly one .c
        #include "dbj_hashmap_element_result.h"

    The #define comes from toplevel/dbj_result.h, whose factories are
    ordinary functions. Everything here is a macro.

    Every dbj_hashmap operation returns one of these. A key that is
    simply not in the map is OK, carrying an element whose key.id is
    HK_EMPTY -- absence is an answer. ERR means the map could not
    answer at all: capacity exhausted.
*/
#include <dbj_required_compile_time.h>

#include "dbj_hashmap_element.h"

#include <stdio.h> /* snprintf, for DBJ_MAKERESULT's err factory */

DBJ_MAKERESULT(HashMapElement);

/* Says nothing about whether the key was found -- a missing key is a
   successful lookup. */
#define dbj_result_is_ok(result_) ((result_).tag == DBJ_RESULT_OK)

#define dbj_result_is_err(result_) ((result_).tag == DBJ_RESULT_ERR)

/* These three read the OK arm, so they are valid only once
   dbj_result_is_ok() has said so. C cannot enforce that. */
#define dbj_result_element(result_) ((result_).HashMapElement_OK.my_value)

/* HK_EMPTY, HK_NULL or HK_VALUE -- the usual question after a get. */
#define dbj_result_state(result_) (dbj_result_element(result_).key.id)

/* Meaningful only when the state is HK_VALUE. */
#define dbj_result_value(result_) (dbj_result_element(result_).val)

/* The ERR arm, valid only after dbj_result_is_err(). Both are
   fixed-size char arrays, so safe to print with %s. */
#define dbj_result_location(result_) ((result_).HashMapElement_ERR.location)

#define dbj_result_message(result_) ((result_).HashMapElement_ERR.message)
