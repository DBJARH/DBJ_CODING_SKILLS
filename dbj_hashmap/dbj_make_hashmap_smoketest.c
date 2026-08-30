/*
    2026AUG30       (c) dbj@dbj.org

    Smoke test for dbj_make_hashmap.h. Exits non-zero on failure, so
    `make test` is a usable check.

    One instantiation, one map. The wider tests -- a second unrelated
    instantiation, and the full-map probe -- sit in the comment at the
    bottom of this file; they are not smoke.
*/
static int check_failures = 0;

#define CHECK(cond_, what_)                  \
    do                                       \
    {                                        \
        if (cond_)                           \
        {                                    \
            printf("  ok    %s\n", (what_)); \
        }                                    \
        else                                 \
        {                                    \
            printf("  FAIL  %s\n", (what_)); \
            check_failures++;                      \
        }                                    \
    } while (0)

#include <dbj_required_compile_time.h>

#define DBJ_MAKERESULT_IMPLEMENTATION
#include "dbj_make_hashmap.h"

#include <stdio.h>

/* Every key type writes its own two operations -- there is no default and
   no "small type" shorthand. dbj_hashmap_mix64 is a hash; the value alone
   is not one. */
[[gnu::const]] static inline uint64_t short_hash(short key)
{
    return dbj_hashmap_mix64((uint64_t)key, 0);
}

[[gnu::const]] static inline bool short_equal(short lhs, short rhs)
{
    return lhs == rhs;
}

DBJ_MAKE_HASHMAP(short, bool)

/* One instantiation in this file, so the short names can simply be
   aliases. With two, these have to become _Generic on the map pointer --
   see the comment at the bottom. */
#define hashmap_count    hashmap_short_bool_count
#define hashmap_get      hashmap_short_bool_get
#define hashmap_set      hashmap_short_bool_set
#define hashmap_set_null hashmap_short_bool_set_null

#define SB DBJ_HASHMAP_ELEMENT_TYPE(short, bool)

/* File scope: 1024 slots is too much for the stack on some hosts.
   Zeroed, which is what makes it an empty map -- no constructor ran. */
static hashmap_short_bool flags_map;

int main(void)
{
    printf("dbj_make_hashmap smoke test\n\n");

    CHECK(hashmap_count(&flags_map) == 0, "a zeroed map is empty");

    /* absence is an answer, not a failure */
    hashmap_short_bool_elementResult missing = hashmap_get(&flags_map, 42);
    CHECK(dbj_result_is_ok(missing), "get of an absent key is OK");
    CHECK(dbj_hashmap_state(SB, missing) == DBJ_SLOT_EMPTY, "absent key reads DBJ_SLOT_EMPTY");

    /* the value type is bool, so false is a perfectly good stored value
       and must not read back as absence */
    CHECK(dbj_result_is_ok(hashmap_set(&flags_map, 42, false)), "set 42 = false");
    hashmap_short_bool_elementResult stored = hashmap_get(&flags_map, 42);
    CHECK(dbj_hashmap_state(SB, stored) == DBJ_SLOT_VALUE, "stored false is VALUE, not EMPTY");
    CHECK(dbj_hashmap_value(SB, stored) == false, "stored false reads back false");
    CHECK(dbj_hashmap_key(SB, stored) == 42, "the element carries its key");

    /* replace, not insert */
    CHECK(dbj_result_is_ok(hashmap_set(&flags_map, 42, true)), "set 42 = true");
    CHECK(dbj_hashmap_value(SB, hashmap_get(&flags_map, 42)) == true, "42 now reads true");
    CHECK(hashmap_count(&flags_map) == 1, "replacing did not add a slot");

    /* the SQL NULL: key present, no value, and probing does not stop */
    CHECK(dbj_result_is_ok(hashmap_set_null(&flags_map, 7)), "set 7 = null");
    CHECK(dbj_hashmap_state(SB, hashmap_get(&flags_map, 7)) == DBJ_SLOT_NULL, "7 reads NULL");
    CHECK(dbj_hashmap_state(SB, hashmap_get(&flags_map, 42)) == DBJ_SLOT_VALUE,
          "42 still found past the null slot");
    CHECK(hashmap_count(&flags_map) == 2, "a null key still counts as present");

    /* negative keys: short_hash sign-extends before mixing, which is fine
       as long as it is consistent */
    CHECK(dbj_result_is_ok(hashmap_set(&flags_map, -1, true)), "set -1 = true");
    CHECK(dbj_hashmap_value(SB, hashmap_get(&flags_map, -1)) == true, "-1 reads back");

    printf("\n%s (%d failure%s)\n", check_failures ? "FAILED" : "passed", check_failures,
           check_failures == 1 ? "" : "s");
    return check_failures != 0;
}

/*
    Beyond smoke test complexity raises bellow. 
    Kept here because the code is the documentation.

    With two instantiations the plain aliases above no longer work; the
    short names have to dispatch on the map pointer. The list is written
    by hand -- a macro cannot extend itself, so DBJ_MAKE_HASHMAP cannot
    accumulate the entries:

    #undef hashmap_set
    #define hashmap_set(map_, key_, val_)                          \
        _Generic((map_),                                           \
            hashmap_short_bool *:      hashmap_short_bool_set,     \
            hashmap_unsigned_double *: hashmap_unsigned_double_set \
        )((map_), (key_), (val_))

    ... and the same again for get, count and set_null.

    A second instantiation in the same translation unit -- two unrelated
    types sharing no storage and no tag:

    [[gnu::const]] static inline uint64_t unsigned_hash(unsigned key)
    {
        return dbj_hashmap_mix64(key, 0);
    }

    [[gnu::const]] static inline bool unsigned_equal(unsigned lhs, unsigned rhs)
    {
        return lhs == rhs;
    }

    DBJ_MAKE_HASHMAP(unsigned, double)

    #define UD DBJ_HASHMAP_ELEMENT_TYPE(unsigned, double)

    static hashmap_unsigned_double weights;

    CHECK(hashmap_unsigned_double_count(&weights) == 0, "the second map is untouched");
    CHECK(dbj_result_is_ok(hashmap_unsigned_double_set(&weights, 42, 0.5)), "set 42 = 0.5");
    CHECK(dbj_hashmap_value(UD, hashmap_unsigned_double_get(&weights, 42)) == 0.5,
          "key 42 in the double map is not key 42 in the bool map");
    CHECK(dbj_hashmap_value(SB, hashmap_short_bool_get(&flags_map, 42)) == true,
          "and the bool map still says true");

    Fill a map, then ask for a key that is not there: no free slot, no
    match, and the probe must stop rather than spin:

    static hashmap_short_bool full_map;

    for (short key = 0; key < DBJ_HASHMAP_SLOTS; key++)
    {
        (void)hashmap_short_bool_set(&full_map, key, true);
    }
    CHECK(hashmap_short_bool_count(&full_map) == DBJ_HASHMAP_SLOTS, "the map filled");
    hashmap_short_bool_elementResult overflow = hashmap_short_bool_get(&full_map, 4242);
    CHECK(dbj_result_is_err(overflow), "a full map reports ERR rather than spinning");
*/
