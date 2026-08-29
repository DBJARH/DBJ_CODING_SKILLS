/*
    2026AUG29       (c) dbj@dbj.org

    Smoke test for dbj_hashmap.h -- proves the folder stands on its own,
    with no demo, no benchmark and no other folder involved.

    Exits non-zero on the first failure, so `make && smoketest` is a
    usable check. Every case here is one the map got wrong at some
    point during 0.5.
*/
#include <dbj_required_compile_time.h>

#define DBJ_MAKERESULT_IMPLEMENTATION
#include "dbj_hashmap.h"

#include <stdio.h>

static int failures = 0;

#define CHECK(cond_, what_)                              \
    do                                                   \
    {                                                    \
        if (cond_)                                       \
        {                                                \
            printf("  ok    %s\n", (what_));             \
        }                                                \
        else                                             \
        {                                                \
            printf("  FAIL  %s\n", (what_));             \
            failures++;                                  \
        }                                                \
    } while (0)

/* The map is 1024 slots of HashMapElement -- too large for the stack
   on some hosts, so file scope. Zeroed, which is what makes it an
   empty map: HK_EMPTY is the zero of hk_id. */
static dbj_hashmap map;
static dbj_hashmap full_map;

static bool is_ok(HashMapElementResult result)
{
    return result.tag == DBJ_RESULT_OK;
}

static hk_id state_of(HashMapElementResult result)
{
    return result.HashMapElement_OK.my_value.key.id;
}

int main(void)
{
    printf("dbj_hashmap smoke test\n\n");

    /* a zeroed map is an empty map -- no constructor was called */
    CHECK(dbj_hashmap_count(&map) == 0, "zeroed map is empty");

    /* absence is an answer, not a failure: OK carrying HK_EMPTY */
    HashMapElementResult miss = dbj_hashmap_get(&map, 42);
    CHECK(is_ok(miss), "get on empty map returns OK, not ERR");
    CHECK(state_of(miss) == HK_EMPTY, "missing key reports HK_EMPTY");

    /* insert and read back */
    (void)dbj_hashmap_set(&map, 42, hash_string_32("forty two"));
    HashMapElementResult hit = dbj_hashmap_get(&map, 42);
    CHECK(is_ok(hit) && state_of(hit) == HK_VALUE, "stored key reports HK_VALUE");
    CHECK(strcmp(hash_string_text(&hit.HashMapElement_OK.my_value.val),
                 "forty two") == 0,
          "stored value reads back");
    CHECK(dbj_hashmap_count(&map) == 1, "one key counted");

    /* overwrite: same slot, new value, still one key */
    (void)dbj_hashmap_set(&map, 42, hash_string_64("replaced"));
    hit = dbj_hashmap_get(&map, 42);
    CHECK(strcmp(hash_string_text(&hit.HashMapElement_OK.my_value.val),
                 "replaced") == 0,
          "overwrite replaces the value");
    CHECK(dbj_hashmap_count(&map) == 1, "overwrite does not add a key");

    /* a HashString of a different size survives the round trip */
    CHECK(hit.HashMapElement_OK.my_value.val.type_id == DBJSTR64,
          "value remembers which dbj_str size it is");

    /* explicit NULL: the key stays present, the value goes */
    (void)dbj_hashmap_set_null(&map, 42);
    hit = dbj_hashmap_get(&map, 42);
    CHECK(is_ok(hit) && state_of(hit) == HK_NULL, "set_null reports HK_NULL");
    CHECK(dbj_hashmap_count(&map) == 1, "a NULL key is still a key");

    /* a NULL entry must not stop a probe: put a second key in, then
       check both are still findable */
    (void)dbj_hashmap_set(&map, 43, hash_string_32("forty three"));
    CHECK(state_of(dbj_hashmap_get(&map, 43)) == HK_VALUE,
          "a key inserted after a NULL is findable");
    CHECK(state_of(dbj_hashmap_get(&map, 42)) == HK_NULL,
          "the NULL key is still findable");

    /* consecutive keys must not degrade: 0..255 all readable */
    static dbj_hashmap run;
    for (KeyType key = 0; key < 256; key++)
    {
        (void)dbj_hashmap_set(&run, key, hash_string_32("x"));
    }
    int found = 0;
    for (KeyType key = 0; key < 256; key++)
    {
        found += state_of(dbj_hashmap_get(&run, key)) == HK_VALUE;
    }
    CHECK(found == 256, "256 consecutive ordinal keys all read back");
    CHECK(dbj_hashmap_count(&run) == 256, "256 keys counted");

    /* the case that used to hang forever: every slot taken, then a
       lookup for a key that is not there. It must return, and it must
       return ERR rather than a wrong answer. */
    for (KeyType key = 0; key < DBJ_HASHMAP_SLOTS; key++)
    {
        (void)dbj_hashmap_set(&full_map, key, hash_string_32("v"));
    }
    CHECK(dbj_hashmap_count(&full_map) == DBJ_HASHMAP_SLOTS, "map fills to capacity");

    HashMapElementResult exhausted = dbj_hashmap_get(&full_map, DBJ_HASHMAP_SLOTS + 1);
    CHECK(exhausted.tag == DBJ_RESULT_ERR, "full-table miss returns ERR, does not hang");

    /* and a key that IS present in a full table still resolves */
    CHECK(state_of(dbj_hashmap_get(&full_map, 7)) == HK_VALUE,
          "full-table hit still resolves");

    printf("\n%s: %d failure(s)\n", failures ? "FAILED" : "PASSED", failures);
    return failures ? 1 : 0;
}
