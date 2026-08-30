/*
    2026AUG29       (c) dbj@dbj.org

    Smoke test for dbj_hashmap.h. Exits non-zero on failure, so
    `make test` is a usable check. Every case here is one the map got
    wrong at some point during 0.5.
*/
#include <dbj_required_compile_time.h>

#define DBJ_MAKERESULT_IMPLEMENTATION
#include "dbj_hashmap.h"

#include <stdio.h>

/* The generated names carry both type arguments, which is correct and
   long. Two local shorthands, in the test only -- the map itself has no
   alias layer. */
typedef hashmap_HashKey_HashString map_t;
typedef hashmap_HashKey_HashString_elementResult map_result;
#define ELEM DBJ_HASHMAP_ELEMENT_TYPE(HashKey, HashString)

#define map_get hashmap_HashKey_HashString_get
#define map_set hashmap_HashKey_HashString_set
#define map_set_null hashmap_HashKey_HashString_set_null
#define map_count hashmap_HashKey_HashString_count

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

/* File scope: 1024 slots of HashMapElement is too much for the stack
   on some hosts. Zeroed, which is what makes it an empty map. */
static map_t map;
static map_t full_map;

int main(void)
{
    printf("dbj_hashmap smoke test\n\n");

    /* a zeroed map is an empty map -- no constructor was called */
    CHECK(map_count(&map) == 0, "zeroed map is empty");

    /* absence is an answer, not a failure: OK carrying DBJ_SLOT_EMPTY */
    map_result miss = map_get(&map, hash_key_ordinal(42));
    CHECK(dbj_result_is_ok(miss), "get on empty map returns OK, not ERR");
    CHECK(dbj_hashmap_state(ELEM,miss) == DBJ_SLOT_EMPTY, "missing key reports DBJ_SLOT_EMPTY");

    /* insert and read back */
    (void)map_set(&map, hash_key_ordinal(42), hash_string_32("forty two"));
    map_result hit = map_get(&map, hash_key_ordinal(42));
    CHECK(dbj_result_is_ok(hit) && dbj_hashmap_state(ELEM,hit) == DBJ_SLOT_VALUE, "stored key reports DBJ_SLOT_VALUE");
    CHECK(strcmp(hash_string_text(&dbj_hashmap_value(ELEM,hit)), "forty two") == 0,
          "stored value reads back");
    CHECK(map_count(&map) == 1, "one key counted");

    /* overwrite: same slot, new value, still one key */
    (void)map_set(&map, hash_key_ordinal(42), hash_string_64("replaced"));
    hit = map_get(&map, hash_key_ordinal(42));
    CHECK(strcmp(hash_string_text(&dbj_hashmap_value(ELEM,hit)), "replaced") == 0,
          "overwrite replaces the value");
    CHECK(map_count(&map) == 1, "overwrite does not add a key");

    /* a HashString of a different size survives the round trip */
    CHECK(dbj_hashmap_value(ELEM,hit).type_id == DBJSTR64,
          "value remembers which dbj_str size it is");

    /* explicit NULL: the key stays present, the value goes */
    (void)map_set_null(&map, hash_key_ordinal(42));
    hit = map_get(&map, hash_key_ordinal(42));
    CHECK(dbj_result_is_ok(hit) && dbj_hashmap_state(ELEM,hit) == DBJ_SLOT_NULL, "set_null reports DBJ_SLOT_NULL");
    CHECK(map_count(&map) == 1, "a NULL key is still a key");

    /* a NULL entry must not stop a probe: put a second key in, then
       check both are still findable */
    (void)map_set(&map, hash_key_ordinal(43), hash_string_32("forty three"));
    CHECK(dbj_hashmap_state(ELEM,map_get(&map, hash_key_ordinal(43))) == DBJ_SLOT_VALUE,
          "a key inserted after a NULL is findable");
    CHECK(dbj_hashmap_state(ELEM,map_get(&map, hash_key_ordinal(42))) == DBJ_SLOT_NULL,
          "the NULL key is still findable");

    /* consecutive keys must not degrade: 0..255 all readable */
    static map_t run;
    for (KeyOrdinal key = 0; key < 256; key++)
    {
        (void)map_set(&run, hash_key_ordinal(key), hash_string_32("x"));
    }
    int found = 0;
    for (KeyOrdinal key = 0; key < 256; key++)
    {
        found += dbj_hashmap_state(ELEM,map_get(&run, hash_key_ordinal(key))) == DBJ_SLOT_VALUE;
    }
    CHECK(found == 256, "256 consecutive ordinal keys all read back");
    CHECK(map_count(&run) == 256, "256 keys counted");

    /* the case that used to hang forever: every slot taken, then a
       lookup for a key that is not there. It must return, and it must
       return ERR rather than a wrong answer. */
    for (KeyOrdinal key = 0; key < DBJ_HASHMAP_SLOTS; key++)
    {
        (void)map_set(&full_map, hash_key_ordinal(key), hash_string_32("v"));
    }
    CHECK(map_count(&full_map) == DBJ_HASHMAP_SLOTS, "map fills to capacity");

    map_result exhausted = map_get(&full_map, hash_key_ordinal(DBJ_HASHMAP_SLOTS + 1));
    CHECK(dbj_result_is_err(exhausted), "full-table miss returns ERR, does not hang");

    /* and a key that IS present in a full table still resolves */
    CHECK(dbj_hashmap_state(ELEM,map_get(&full_map, hash_key_ordinal(7))) == DBJ_SLOT_VALUE,
          "full-table hit still resolves");

    /* ---- string keys: the same map, a second key kind ---- */

    static map_t strmap;

    (void)map_set(&strmap, hash_key_string("alpha"), hash_string_32("first"));
    (void)map_set(&strmap, hash_key_string("beta"), hash_string_32("second"));

    CHECK(dbj_hashmap_state(ELEM,map_get(&strmap, hash_key_string("alpha"))) == DBJ_SLOT_VALUE,
          "string key reads back");
    map_result beta = map_get(&strmap, hash_key_string("beta"));
    CHECK(strcmp(hash_string_text(&dbj_hashmap_value(ELEM,beta)), "second") == 0,
          "string key finds its own value, not the other one");
    CHECK(dbj_hashmap_state(ELEM,map_get(&strmap, hash_key_string("gamma"))) == DBJ_SLOT_EMPTY,
          "absent string key reports DBJ_SLOT_EMPTY");
    CHECK(map_count(&strmap) == 2, "two string keys counted");

    /* the point of the kind tag: an ordinal and a string are never the
       same key, whatever their bits happen to be */
    (void)map_set(&strmap, hash_key_ordinal(0), hash_string_32("ordinal zero"));
    CHECK(map_count(&strmap) == 3, "an ordinal key does not collide with a string key");
    map_result zero = map_get(&strmap, hash_key_ordinal(0));
    CHECK(strcmp(hash_string_text(&dbj_hashmap_value(ELEM,zero)), "ordinal zero") == 0,
          "mixed-kind map keeps both kinds apart");

    /* a string key longer than KeyString truncates -- two keys sharing
       the first 32 bytes are one key, and that is the documented cost */
    (void)map_set(&strmap,
                          hash_key_string("0123456789012345678901234567890123456789"),
                          hash_string_32("truncated"));
    CHECK(dbj_hashmap_state(ELEM,map_get(&strmap,
              hash_key_string("01234567890123456789012345678901"))) == DBJ_SLOT_VALUE,
          "a string key truncates at KeyString capacity");

    /* ---- slice keys: a third kind, the first that points at text it
       does not hold ---- */

    static map_t slicemap;

    (void)map_set(&slicemap, hash_key_slice(DBJ_SS("alpha")), hash_string_32("first"));
    (void)map_set(&slicemap, hash_key_slice(DBJ_SS("beta")), hash_string_32("second"));

    CHECK(dbj_hashmap_state(ELEM,map_get(&slicemap, hash_key_slice(DBJ_SS("alpha")))) == DBJ_SLOT_VALUE,
          "slice key reads back");
    map_result slice_beta = map_get(&slicemap, hash_key_slice(DBJ_SS("beta")));
    CHECK(strcmp(hash_string_text(&dbj_hashmap_value(ELEM,slice_beta)), "second") == 0,
          "slice key finds its own value");
    CHECK(dbj_hashmap_state(ELEM,map_get(&slicemap, hash_key_slice(DBJ_SS("gamma")))) == DBJ_SLOT_EMPTY,
          "absent slice key reports DBJ_SLOT_EMPTY");

    /* a slice matches by content, not by pointer: the same text from a
       different address is the same key */
    char elsewhere[] = "alpha";
    dbj_str_slice copy = {elsewhere, 5};
    CHECK(dbj_hashmap_state(ELEM,map_get(&slicemap, hash_key_slice(copy))) == DBJ_SLOT_VALUE,
          "slice key matches by content, not by address");

    /* no 32-byte ceiling: a slice key longer than KeyString stays
       distinct where a KT_STRING key would have truncated */
    (void)map_set(&slicemap,
                          hash_key_slice(DBJ_SS("0123456789012345678901234567890123456789")),
                          hash_string_32("long one"));
    CHECK(dbj_hashmap_state(ELEM,map_get(&slicemap,
              hash_key_slice(DBJ_SS("01234567890123456789012345678901")))) == DBJ_SLOT_EMPTY,
          "a slice key does not truncate");

    /* text copied into an arena: the lifetime answered by something
       other than a literal */
    static char block[4096];
    dbj_arena arena = dbj_arena_make(block, sizeof(block));

    char stack_text[] = "from the stack";
    HashKeyResult arena_key = hash_key_slice_copy(&arena, stack_text);
    CHECK(dbj_result_is_ok(arena_key), "a roomy arena yields a key");
    (void)map_set(&slicemap, dbj_result_hash_key(arena_key),
                          hash_string_32("arena held"));

    /* scribble over the stack buffer -- the arena copy is a copy, so
       the key must still be found and still be that text */
    memset(stack_text, 'x', sizeof(stack_text) - 1);
    map_result held =
        map_get(&slicemap, hash_key_slice(DBJ_SS("from the stack")));
    CHECK(dbj_hashmap_state(ELEM,held) == DBJ_SLOT_VALUE,
          "an arena-held slice key survives its source going away");
    CHECK(strcmp(hash_string_text(&dbj_hashmap_value(ELEM,held)), "arena held") == 0,
          "the arena copy carries the right value");

    /* an exhausted arena says so. This used to hand back a zero-length
       KT_SLICE key, indistinguishable from a key whose text really is
       empty -- so every failed copy became the same key, and inserting
       it quietly overwrote the last one. */
    static char tiny_block[8];
    dbj_arena tiny = dbj_arena_make(tiny_block, sizeof(tiny_block));
    HashKeyResult too_big =
        hash_key_slice_copy(&tiny, "considerably longer than eight bytes");
    CHECK(dbj_result_is_err(too_big), "an exhausted arena reports ERR, not a key");
    CHECK(dbj_result_message(HashKey, too_big)[0] != '\0',
          "the exhaustion carries a message");
    CHECK(strcmp(dbj_result_location(HashKey, too_big), "hash_key_slice_copy") == 0,
          "the exhaustion names where it surfaced");

    /* the three kinds are three keys, never one */
    (void)map_set(&slicemap, hash_key_string("alpha"), hash_string_32("owned alpha"));
    map_result owned = map_get(&slicemap, hash_key_string("alpha"));
    map_result pointing = map_get(&slicemap, hash_key_slice(DBJ_SS("alpha")));
    CHECK(strcmp(hash_string_text(&dbj_hashmap_value(ELEM,owned)), "owned alpha") == 0 &&
              strcmp(hash_string_text(&dbj_hashmap_value(ELEM,pointing)), "first") == 0,
          "a held key and a pointing key of the same text stay apart");

    printf("\n%s: %d failure(s)\n", failures ? "FAILED" : "PASSED", failures);
    return failures ? 1 : 0;
}
