/*
    2026AUG28       (c) dbj@dbj.org

    dbj arena + hashmap + hashtrie
    ------------------------------
    A dbj rework of Chris Wellons' core-lib techniques:

        https://nullprogram.com/blog/2025/01/19/

    The unmodified original sits beside this file as
    yet-another-good-corelib.c -- reference only. What was kept, what
    was dropped, and why, is in readme.md in this folder.

    Three data structures, one memory strategy:

        dbj_arena     bump allocator -- the only allocation in the
                      program is the one block backing it
        dbj_hashmap   flat, fixed-capacity, open-address table (MSI)
        dbj_hashtrie  4-ary trie, unbounded, never resizes or rehashes

    All three are usable in their all-bits-zero state, so none of them
    has a constructor. That is the point of the design, not an accident
    of it: an empty trie is a null pointer, an empty map is zeroed
    memory, an empty slice is {0}.
*/
#include <dbj_required_compile_time.h>

#include <dbj_clintro.h>
#include <dbj_defer.h>
#include <dbj_macros.h>
#include <dbj_str_slice.h>

#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DBJ_APP_NAME "dbj_arena_hashmap_hashtrie"
#define DBJ_APP_VERSION "0.1.0"

/* Element count of a true array -- never a pointer -- as a signed size. */
#define dbj_countof(array_) ((ptrdiff_t)(sizeof(array_) / sizeof(*(array_))))

/* Allocate count objects of type type_ from arena_, zeroed and aligned.
   The cast turns a type mismatch into a diagnostic, and sizeof/_Alignof
   are taken from the type, so the caller cannot get the arithmetic
   wrong. */
#define dbj_arena_new(arena_, count_, type_) \
    (type_ *)dbj_arena_alloc((arena_), (count_), sizeof(type_), _Alignof(type_))

/* -------------------------------------------------------------------
   dbj_arena -- bump allocator
   ------------------------------------------------------------------- */

/* The whole allocator: two pointers. `beg` is the bump cursor, `end`
   the hard limit.

   Passing a dbj_arena *by value* yields a scratch arena: the callee
   bumps its own copy, and every allocation it made is gone the moment
   it returns. That is the entire lifetime story -- there is no free. */
typedef struct
{
    char *beg;
    char *end;
} dbj_arena;

/* Factory: wrap an already-owned block. Caller keeps ownership of the
   block; the arena only ever points into it. */
static dbj_arena dbj_arena_make(char *block, ptrdiff_t size)
{
    return (dbj_arena){block, block + size};
}

/* Bump `count` objects of `size` bytes off the front, aligned and
   zeroed. Zeroed is what lets every structure here start valid at
   all-bits-zero.

   TODO(post-1.0): this assert is where an OOM policy goes -- see
   toplevel/dbj_result.h. Not used yet: a DBJ_RESULT carries two 512
   byte char arrays, which is right at an application boundary and
   quite wrong in an allocator this hot. */
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
   The type itself, with equals and the seeded hash, lives in
   toplevel/dbj_str_slice.h -- it has no opinion about allocation. What
   follows is only the part that does. */

/* Duplicate `text` into the arena. The `if (result.len)` guard is not
   redundant: memcpy forbids a null source even for a zero count, and
   `text` may be a zero slice. */
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

/* head + tail, in place whenever head is already the most recent thing
   in the arena: then tail's copy lands immediately behind it and only
   the length has to grow. Otherwise head is relocated to the bump
   pointer first. This is what makes repeated concatenation cost
   O(total length) rather than O(n^2). */
static dbj_str_slice dbj_str_slice_concat(dbj_arena *arena, dbj_str_slice head, dbj_str_slice tail)
{
    if (!head.data || head.data + head.len != arena->beg)
    {
        head = dbj_str_slice_copy(arena, head);
    }
    head.len += dbj_str_slice_copy(arena, tail).len;
    return head;
}

/* printf straight into the arena, no intermediate buffer: the formatted
   text is returned as a slice pointing at it. Only the bytes actually
   written are committed -- the NUL vsnprintf appends is left sitting
   past `beg`, uncommitted.

   gnu_printf, not printf: on MinGW the plain `printf` archetype checks
   against the old msvcrt, which rejects %zu and %td. This toolchain is
   UCRT (see the user CLAUDE.md), which supports both. */
[[gnu::format(gnu_printf, 2, 3)]] static dbj_str_slice
dbj_arena_print(dbj_arena *arena, const char *fmt, ...)
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
   dbj_slice -- dynamic array
   -------------------------------------------------------------------
   One slice type per element type: C has no templates, and the repo
   forbids inventing a generic layer to fake them. {0} is a valid empty
   slice.

   Wellons offers a second, macro-based mechanism (`push`) that works
   on any data/len/cap struct. It is not here: it evaluates its
   argument several times, and the repo's rule is no abstraction beyond
   what the file demonstrates. */
typedef struct
{
    dbj_str_slice *data;
    ptrdiff_t len;
    ptrdiff_t cap;
} dbj_str_slice_array;

/* Relocate an array to the bump pointer, element by element. */
static dbj_str_slice_array dbj_str_slice_array_clone(dbj_arena *arena, dbj_str_slice_array source)
{
    dbj_str_slice_array result = {0};
    result.len = result.cap = source.len;
    result.data = dbj_arena_new(arena, source.len, dbj_str_slice);
    DBJ_LOOP_AS(i, (size_t)source.len)
    {
        result.data[i] = source.data[i];
    }
    return result;
}

/* Append one element, growing if full. Takes the array by value and
   returns the updated header, so callers write

       words = dbj_str_slice_array_append(&arena, words, word);

   Growth is the same in-place-if-possible trick as concat: an array
   that already sits at the bump pointer is extended where it lies,
   otherwise it is relocated first. Capacity doubles, from 4. */
static dbj_str_slice_array
dbj_str_slice_array_append(dbj_arena *arena, dbj_str_slice_array array, dbj_str_slice value)
{
    if (array.len == array.cap)
    {
        if (!array.data || (void *)(array.data + array.len) != arena->beg)
        {
            array = dbj_str_slice_array_clone(arena, array);
        }
        ptrdiff_t extend = array.cap ? array.cap : 4;
        dbj_arena_new(arena, extend, dbj_str_slice); /* grow the backing buffer */
        array.cap += extend;
    }
    array.data[array.len++] = value;
    return array;
}

/* -------------------------------------------------------------------
   dbj_hashmap -- flat, fixed capacity, open addressing (MSI)
   ------------------------------------------------------------------- */

/* Mask-Step-Index: two parallel fixed arrays. A null key pointer marks
   an empty slot, so a zeroed dbj_hashmap is an empty map.

   Capacity is a hard ceiling. The map does not resize, and it does not
   detect its own overflow -- insert past capacity and the probe loop
   below spins forever. That ceiling is the trade being demonstrated;
   dbj_hashtrie is the answer when the key count is unbounded. */
enum
{
    DBJ_HASHMAP_EXP = 10
}; /* 1024 slots -- keep to ~1000 unique keys */

typedef struct
{
    dbj_str_slice keys[1 << DBJ_HASHMAP_EXP];
    dbj_str_slice vals[1 << DBJ_HASHMAP_EXP];
} dbj_hashmap;

/* Lookup *and* insert: returns the address of the value slot for `key`,
   claiming an empty slot when the key is absent. A null `.data` in the
   returned slice therefore means "was not present" -- the caller
   assigns through the pointer to insert.

   Double hashing: low bits index, high bits (the well-mixed end of a
   multiplicative hash) supply an odd step. Odd is coprime with a
   power-of-two table size, so probing visits every slot.

   The map's own address is the hash seed. ASLR randomises it, so an
   attacker cannot precompute colliding keys, and no seed has to be
   stored or threaded through anywhere. */
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

/* 4-ary trie node. The map *is* the root pointer: a null
   dbj_hashtrie * is an empty map, which is why insertion takes a
   dbj_hashtrie ** -- it has to be able to write the root itself.

   Nodes are never moved and never rehashed, so a pointer handed out by
   a lookup stays valid for as long as the arena does. */
typedef struct dbj_hashtrie dbj_hashtrie;
struct dbj_hashtrie
{
    dbj_hashtrie *child[4];
    dbj_str_slice key;
    dbj_str_slice value;
};

/* Lookup and insert, with the mode chosen by the arena argument:
   nullptr means pure lookup (returns nullptr when absent), a real arena
   means insert-if-missing.

   Two hash bits are consumed per level, taken from the top (h >> 62,
   then h <<= 2). Depth is O(log4 n) for well-distributed keys.

   The root node's address seeds the hash, which looks circular but is
   not: the very first insert does not consult the hash at all -- it
   simply becomes the root. By the second insert a seed exists. This
   also means the trie survives being copied to another root variable,
   which a fixed per-call seed would not. */
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

/* Collect every key in the trie into one array, iteratively.

   Iteratively, not recursively: a lopsided trie -- hostile keys, or
   simple bad luck somewhere north of 100k entries -- would exhaust the
   call stack. Here depth costs arena instead of call frames.

   `initial` is the trick worth noticing: a plain automatic array used
   as the stack's first storage, so the common case leaves no litter in
   the arena at all. Only if 16 frames prove too few does the append
   above notice the stack is no longer at the bump pointer and relocate
   it. */
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
    DBJ_LOOP_AS(i, (size_t)source.len)
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

static dbj_str_slice_array dbj_hashtrie_keys(dbj_hashtrie *trie, dbj_arena *arena)
{
    dbj_str_slice_array result = {0};

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
            result = dbj_str_slice_array_append(arena, result, top->node->key);
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
   Each takes its dbj_arena *by value*. They therefore all start from
   the same empty arena, none can see another's allocations, and none
   of them frees anything. */

enum
{
    DBJ_DEMO_COUNT = 256,
    DBJ_DEMO_PROBE = 100
};

static void dbj_hashmap_demo(dbj_arena scratch)
{
    dbj_hashmap *map = dbj_arena_new(&scratch, 1, dbj_hashmap);

    DBJ_LOOP_AS(i, DBJ_DEMO_COUNT)
    {
        dbj_str_slice key = dbj_arena_print(&scratch, "key%zu", i);
        dbj_str_slice value = dbj_arena_print(&scratch, "value%zu", i);
        *dbj_hashmap_lookup(map, key) = value;
    }

    dbj_str_slice found = *dbj_hashmap_lookup(map, DBJ_SS("key100"));
    printf("  hashmap   key100 -> %.*s\n", (int)found.len, found.data);
}

static void dbj_hashtrie_demo(dbj_arena scratch)
{
    dbj_hashtrie *trie = nullptr; /* an empty trie is a null pointer */

    DBJ_LOOP_AS(i, DBJ_DEMO_COUNT)
    {
        dbj_str_slice key = dbj_arena_print(&scratch, "key%zu", i);
        dbj_str_slice value = dbj_arena_print(&scratch, "value%zu", i);
        *dbj_hashtrie_lookup(&trie, key, &scratch) = value;
    }

    /* nullptr arena -- lookup only, no insertion */
    dbj_str_slice *found = dbj_hashtrie_lookup(&trie, DBJ_SS("key100"), nullptr);
    printf("  hashtrie  key100 -> %.*s\n", (int)found->len, found->data);

    dbj_str_slice_array keys = dbj_hashtrie_keys(trie, &scratch);
    printf("  hashtrie  walked %td keys\n", keys.len);
}

static void dbj_slice_demo(dbj_arena scratch)
{
    dbj_str_slice_array words = {0}; /* an empty array is {0} */

    DBJ_LOOP_AS(i, DBJ_DEMO_COUNT)
    {
        dbj_str_slice word = dbj_arena_print(&scratch, "word%zu", i);
        words = dbj_str_slice_array_append(&scratch, words, word);
    }

    dbj_str_slice element = words.data[DBJ_DEMO_PROBE];
    printf("  slice     [%d]    -> %.*s\n", DBJ_DEMO_PROBE, (int)element.len, element.data);
}

static void dbj_concat_demo(dbj_arena scratch)
{
    /* four concats, all in place: each piece is appended straight
       behind the previous one, because `pair` never stops being the
       most recent thing in the arena */
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
    /* the one and only allocation in this program, and the one and
       only thing that has to be released -- everything below is bump
       allocated out of it and freed by scope */
    defer { free(block); };

    dbj_arena arena = dbj_arena_make(block, DBJ_ARENA_SIZE);

    dbj_hashmap_demo(arena);
    dbj_hashtrie_demo(arena);
    dbj_slice_demo(arena);
    dbj_concat_demo(arena);

    return EXIT_SUCCESS;
}
