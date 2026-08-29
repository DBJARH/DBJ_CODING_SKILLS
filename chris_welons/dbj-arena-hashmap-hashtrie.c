/*
    2026AUG28       (c) dbj@dbj.org

    dbj arena + hashmap + hashtrie -- a rework of Chris Wellons'
    core-lib techniques. https://nullprogram.com/blog/2025/01/19/

    His unmodified original is beside this file as
    yet-another-good-corelib.c. What was kept and dropped, and why, is
    in readme.md.

    All-bits-zero is a valid value of every type here, so none has a
    constructor: an empty trie is a null pointer, an empty map is
    zeroed memory, an empty dbj_strings is {0}.
*/
#include <dbj_required_compile_time.h>

#include <dbj_clintro.h>
#include <dbj_defer.h>
#include <dbj_str_slice.h>

#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Name and version for dbj_clintro's banner, and for any message this
   app prints about itself.

   Hardcoded on purpose. There is no build-time version generation in
   this repo and none is wanted here -- dbj_the_game/build_timestamp.inc
   does that, and it is overkill for a single-file example. The record
   that counts is the git tag and the `version:` front matter in
   readme.md; this string is a courtesy to whoever is looking at the
   terminal, so bump it by hand when the file changes meaningfully. */
#define DBJ_APP_NAME "dbj_arena_hashmap_hashtrie"
#define DBJ_APP_VERSION "0.1.0"

/* Also in toplevel/dbj_macros.h; kept local so this file stands alone. */
#define dbj_countof(array_) ((ptrdiff_t)(sizeof(array_) / sizeof(*(array_))))

/* The cast makes a type mismatch a diagnostic; sizeof/_Alignof come
   from the type, so the caller cannot get the arithmetic wrong. */
#define dbj_arena_new(arena_, count_, type_) \
    (type_ *)dbj_arena_alloc((arena_), (count_), sizeof(type_), _Alignof(type_))

/* -------------------------------------------------------------------
   dbj_arena -- bump allocator
   ------------------------------------------------------------------- */

/* Passed *by value* this is a scratch arena: the callee bumps its own
   copy, and everything it allocated is gone when it returns. That is
   the whole lifetime story -- there is no free. */
typedef struct
{
    char *beg;
    char *end;
} dbj_arena;

/* The caller keeps the block; the arena only points into it. */
static dbj_arena dbj_arena_make(char *block, ptrdiff_t size)
{
    return (dbj_arena){block, block + size};
}

/* Zeroed, which is what lets every type here start valid at
   all-bits-zero.

   TODO(post-1.0): the assert is where an OOM policy goes. A DBJ_RESULT
   carries two 512-byte char arrays -- right at an app boundary, wrong
   in an allocator this hot. */
static void *dbj_arena_alloc(dbj_arena *arena, ptrdiff_t count, ptrdiff_t size, ptrdiff_t align)
{
    /* bytes needed to round beg up to `align` -- align is a power of two */
    ptrdiff_t pad = -(uintptr_t)arena->beg & (align - 1);
    /* division, not multiplication, so the check itself cannot overflow */
    assert(count < (arena->end - arena->beg - pad) / size);
    void *result = arena->beg + pad;
    arena->beg += pad + count * size;
    return memset(result, 0, count * size);
}

/* -------------------------------------------------------------------
   dbj_str_slice helpers that need an arena
   -------------------------------------------------------------------
   The type, equals and the seeded hash are in
   toplevel/dbj_str_slice.h; only the allocating part is here. */

/* The `if (result.len)` guard is not redundant: memcpy forbids a null
   source even for a zero count, and `text` may be a zero slice. */
static dbj_str_slice dbj_str_slice_copy(dbj_arena *arena, dbj_str_slice text)
{
    dbj_str_slice result = text;
    result.data = dbj_arena_new(arena, text.len, char);
    if (result.len)
    {
        memcpy(result.data, text.data, result.len);
    }
    return result;
}

/* In place whenever head is the most recent thing in the arena:
   tail's copy lands behind it and only the length grows. This is what
   makes repeated concatenation O(total length), not O(n^2). */
static dbj_str_slice dbj_str_slice_concat(dbj_arena *arena, dbj_str_slice head, dbj_str_slice tail)
{
    if (!head.data || head.data + head.len != arena->beg)
    {
        head = dbj_str_slice_copy(arena, head);
    }
    head.len += dbj_str_slice_copy(arena, tail).len;
    return head;
}

/* printf straight into the arena, no intermediate buffer. Only the
   bytes written are committed -- the NUL vsnprintf appends is left
   past `beg`, uncommitted. */
static dbj_str_slice dbj_arena_print(dbj_arena *arena, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    ptrdiff_t capacity = arena->end - arena->beg;
    int written = vsnprintf(arena->beg, (size_t)capacity, fmt, args);
    va_end(args);

    dbj_str_slice result = {0};
    if (written < 0 || written >= capacity)
    {
        return result; /* TODO(post-1.0): OOM policy, as in dbj_arena_alloc */
    }
    result.data = arena->beg;
    result.len = written;
    arena->beg += result.len;
    return result;
}

/* -------------------------------------------------------------------
   dbj_strings -- growable array of dbj_str_slice
   -------------------------------------------------------------------
   One such type per element type: C has no templates. {0} is a valid
   empty dbj_strings.

   Wellons' `push` macro works on any data/len/cap struct and is not
   here: it evaluates its argument several times. */
typedef struct
{
    dbj_str_slice *data;
    ptrdiff_t len;
    ptrdiff_t cap;
} dbj_strings;

/* Relocate to the bump pointer, element by element. */
static dbj_strings dbj_strings_clone(dbj_arena *arena, dbj_strings source)
{
    dbj_strings result = {0};
    result.len = result.cap = source.len;
    result.data = dbj_arena_new(arena, source.len, dbj_str_slice);
    for (ptrdiff_t i = 0; i < source.len; i++)
    {
        result.data[i] = source.data[i];
    }
    return result;
}

/* By value in, updated header out, so callers write
   `words = dbj_strings_append(&arena, words, word)`.

   Growth is concat's in-place-if-possible trick: storage already at
   the bump pointer is extended where it lies. Doubles, from 4. */
static dbj_strings
dbj_strings_append(dbj_arena *arena, dbj_strings strings, dbj_str_slice value)
{
    if (strings.len == strings.cap)
    {
        if (!strings.data || (void *)(strings.data + strings.len) != arena->beg)
        {
            strings = dbj_strings_clone(arena, strings);
        }
        ptrdiff_t extend = strings.cap ? strings.cap : 4;
        dbj_arena_new(arena, extend, dbj_str_slice); /* grow the backing buffer */
        strings.cap += extend;
    }
    strings.data[strings.len++] = value;
    return strings;
}

/* -------------------------------------------------------------------
   dbj_hashmap -- flat, fixed capacity, open addressing (MSI)
   ------------------------------------------------------------------- */

/* Mask-Step-Index: two parallel fixed arrays, a null key pointer
   marking an empty slot.

   Capacity is a hard ceiling and overflow is not detected -- insert
   past it and the probe loop spins forever. That is the trade being
   shown; dbj_hashtrie is the answer for unbounded key counts. */
enum
{
    DBJ_HASHMAP_EXP = 10
}; /* 1024 slots -- keep to ~1000 unique keys */

typedef struct
{
    dbj_str_slice keys[1 << DBJ_HASHMAP_EXP];
    dbj_str_slice vals[1 << DBJ_HASHMAP_EXP];
} dbj_hashmap;

/* Lookup *and* insert: returns the address of the value slot,
   claiming an empty one if the key is absent. A null `.data` in the
   returned slice means "was not present"; the caller assigns.

   Double hashing: low bits index, high bits (better mixed) supply an
   odd step, coprime with a power-of-two size, so probing visits every
   slot.

   The map's own address seeds the hash -- ASLR randomises it, so
   colliding keys cannot be precomputed and no seed is stored. */
static dbj_str_slice *dbj_hashmap_lookup(dbj_hashmap *map, dbj_str_slice key)
{
    uint64_t hash = dbj_str_slice_hash(key, (uintptr_t)map);
    uint32_t mask = (1 << DBJ_HASHMAP_EXP) - 1;
    uint32_t step = (uint32_t)(hash >> (64 - DBJ_HASHMAP_EXP)) | 1;
    for (int32_t i = (int32_t)hash;;)
    {
        i = (i + step) & mask;
        if (!map->keys[i].data)
        {
            map->keys[i] = key;
            return map->vals + i;
        }
        if (dbj_str_slice_equals(map->keys[i], key))
        {
            return map->vals + i;
        }
    }
}

/* -------------------------------------------------------------------
   dbj_hashtrie -- unbounded map, never resizes, never rehashes
   ------------------------------------------------------------------- */

/* The map *is* the root pointer, which is why insertion takes a
   dbj_hashtrie ** -- it must be able to write the root itself.

   Nodes are never moved or rehashed, so a pointer handed out by a
   lookup stays valid as long as the arena does. */
typedef struct dbj_hashtrie dbj_hashtrie;
struct dbj_hashtrie
{
    dbj_hashtrie *child[4];
    dbj_str_slice key;
    dbj_str_slice value;
};

/* The arena argument picks the mode: nullptr is lookup-only, a real
   arena inserts if missing. Two hash bits per level, from the top;
   depth is O(log4 n) for well-distributed keys.

   Seeding on the root's address looks circular but is not: the first
   insert never consults the hash, it simply becomes the root, and by
   the second a seed exists. It also survives the trie being copied to
   another root variable, which a fixed per-call seed would not. */
static dbj_str_slice *dbj_hashtrie_lookup(dbj_hashtrie **trie, dbj_str_slice key, dbj_arena *arena)
{
    uint64_t seed = trie ? (uintptr_t)*trie : 0;
    for (uint64_t hash = dbj_str_slice_hash(key, seed); *trie; hash <<= 2)
    {
        if (dbj_str_slice_equals(key, (*trie)->key))
        {
            return &(*trie)->value;
        }
        trie = &(*trie)->child[hash >> 62];
    }
    if (!arena)
    {
        return nullptr;
    }
    *trie = dbj_arena_new(arena, 1, dbj_hashtrie);
    (*trie)->key = key;
    return &(*trie)->value;
}

/* Every key in the trie, iteratively: a lopsided trie -- hostile
   keys, or bad luck north of 100k entries -- would exhaust the call
   stack. Depth costs arena here instead of call frames.

   `initial` is the trick worth noticing: an automatic array as the
   stack's first storage, so the common case leaves no litter in the
   arena. Only if 16 frames prove too few does push relocate it. */
typedef struct
{
    dbj_hashtrie *node;
    int index;
} dbj_hashtrie_frame;

typedef struct
{
    dbj_hashtrie_frame *data;
    ptrdiff_t len;
    ptrdiff_t cap;
} dbj_hashtrie_stack;

static dbj_hashtrie_stack dbj_hashtrie_stack_clone(dbj_arena *arena, dbj_hashtrie_stack source)
{
    dbj_hashtrie_stack result = {0};
    result.len = result.cap = source.len;
    result.data = dbj_arena_new(arena, source.len, dbj_hashtrie_frame);
    for (ptrdiff_t i = 0; i < source.len; i++)
    {
        result.data[i] = source.data[i];
    }
    return result;
}

static dbj_hashtrie_stack
dbj_hashtrie_stack_push(dbj_arena *arena, dbj_hashtrie_stack stack, dbj_hashtrie_frame frame)
{
    if (stack.len == stack.cap)
    {
        if (!stack.data || (void *)(stack.data + stack.len) != arena->beg)
        {
            stack = dbj_hashtrie_stack_clone(arena, stack);
        }
        ptrdiff_t extend = stack.cap ? stack.cap : 4;
        dbj_arena_new(arena, extend, dbj_hashtrie_frame);
        stack.cap += extend;
    }
    stack.data[stack.len++] = frame;
    return stack;
}

static dbj_strings dbj_hashtrie_keys(dbj_hashtrie *trie, dbj_arena *arena)
{
    dbj_strings result = {0};

    dbj_hashtrie_frame initial[16]; /* small size optimisation */
    dbj_hashtrie_stack stack = {initial, 0, dbj_countof(initial)};

    stack = dbj_hashtrie_stack_push(arena, stack, (dbj_hashtrie_frame){trie, 0});
    while (stack.len)
    {
        dbj_hashtrie_frame *top = stack.data + stack.len - 1;

        if (!top->node)
        {
            stack.len--;
        }
        else if (top->index == dbj_countof(top->node->child))
        {
            /* all four children visited: emit this node, then pop */
            result = dbj_strings_append(arena, result, top->node->key);
            stack.len--;
        }
        else
        {
            int i = top->index++;
            stack = dbj_hashtrie_stack_push(arena, stack, (dbj_hashtrie_frame){top->node->child[i], 0});
        }
    }

    return result;
}

/* -------------------------------------------------------------------
   Demos
   -------------------------------------------------------------------
   Each takes its dbj_arena *by value*, so all four start from the same
   empty arena and none sees another's allocations. */

enum
{
    DBJ_DEMO_COUNT = 256,
    DBJ_DEMO_PROBE = 100
};

static void dbj_hashmap_demo(dbj_arena scratch)
{
    dbj_hashmap *map = dbj_arena_new(&scratch, 1, dbj_hashmap);

    for (int i = 0; i < DBJ_DEMO_COUNT; i++)
    {
        dbj_str_slice key = dbj_arena_print(&scratch, "key%d", i);
        dbj_str_slice value = dbj_arena_print(&scratch, "value%d", i);
        *dbj_hashmap_lookup(map, key) = value;
    }

    dbj_str_slice found = *dbj_hashmap_lookup(map, DBJ_SS("key100"));
    printf("  hashmap   key100 -> %.*s\n", (int)found.len, found.data);
}

static void dbj_hashtrie_demo(dbj_arena scratch)
{
    dbj_hashtrie *trie = nullptr; /* an empty trie is a null pointer */

    for (int i = 0; i < DBJ_DEMO_COUNT; i++)
    {
        dbj_str_slice key = dbj_arena_print(&scratch, "key%d", i);
        dbj_str_slice value = dbj_arena_print(&scratch, "value%d", i);
        *dbj_hashtrie_lookup(&trie, key, &scratch) = value;
    }

    /* nullptr arena -- lookup only, no insertion */
    dbj_str_slice *found = dbj_hashtrie_lookup(&trie, DBJ_SS("key100"), nullptr);
    printf("  hashtrie  key100 -> %.*s\n", (int)found->len, found->data);

    dbj_strings keys = dbj_hashtrie_keys(trie, &scratch);
    printf("  hashtrie  walked %d keys\n", (int)keys.len);
}

static void dbj_slice_demo(dbj_arena scratch)
{
    dbj_strings words = {0}; /* an empty dbj_strings is {0} */

    for (int i = 0; i < DBJ_DEMO_COUNT; i++)
    {
        dbj_str_slice word = dbj_arena_print(&scratch, "word%d", i);
        words = dbj_strings_append(&scratch, words, word);
    }

    dbj_str_slice element = words.data[DBJ_DEMO_PROBE];
    printf("  slice     [%d]    -> %.*s\n", DBJ_DEMO_PROBE, (int)element.len, element.data);
}

static void dbj_concat_demo(dbj_arena scratch)
{
    /* four concats, all in place: `pair` never stops being the most
       recent thing in the arena */
    dbj_str_slice pair = dbj_str_slice_copy(&scratch, DBJ_SS("PATH"));
    pair = dbj_str_slice_concat(&scratch, pair, DBJ_SS("="));
    pair = dbj_str_slice_concat(&scratch, pair, DBJ_SS("/usr/bin"));
    pair = dbj_str_slice_concat(&scratch, pair, DBJ_SS(":/bin"));

    printf("  concat            -> %.*s\n", (int)pair.len, pair.data);
}

/* -------------------------------------------------------------------
   main
   ------------------------------------------------------------------- */

enum
{
    DBJ_ARENA_SIZE = 1 << 24
}; /* 16 MiB backs the whole program */

int main(int argc, char *argv[static argc + 1])
{
    (void)argv;
    dbj_clintro(DBJ_APP_NAME, DBJ_APP_VERSION);

    char *block = malloc(DBJ_ARENA_SIZE);
    if (!block)
    {
        fprintf(stderr, "%s: cannot allocate %d bytes\n", DBJ_APP_NAME, DBJ_ARENA_SIZE);
        return EXIT_FAILURE;
    }
    /* the one allocation here, and the one thing to be released */
    defer { free(block); };

    dbj_arena arena = dbj_arena_make(block, DBJ_ARENA_SIZE);

    dbj_hashmap_demo(arena);
    dbj_hashtrie_demo(arena);
    dbj_slice_demo(arena);
    dbj_concat_demo(arena);

    return EXIT_SUCCESS;
}
