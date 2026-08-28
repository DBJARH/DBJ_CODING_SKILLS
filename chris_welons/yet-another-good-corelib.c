// https://gist.github.com/skeeto/42d8a23871642696b6b8de30d9222328
// Examples of quick hash tables and dynamic arrays in C
// https://nullprogram.com/blog/2025/01/19/
// This is free and unencumbered software released into the public domain.
//
// Companion source for yet-another-good-corelib.md in this folder.
// Four independent demos, each handed a *by-value copy* of the arena so
// everything it allocates is discarded when it returns:
//
//   msi_demo       flat (Mask-Step-Index) open-address hash table
//   hashtrie_demo  4-ary hash trie, unbounded key count
//   push_demo      dynamic array via the push() macro
//   append_demo    dynamic array via a value-returning append()
//
// Comments added by ZED; code is Wellons' original.
#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Allocate n objects of type t from arena a, zeroed, correctly aligned.
// The cast is what makes a type mismatch a diagnostic rather than a
// silent bug, and sizeof/_Alignof come from the type, so the caller can
// never get the size arithmetic wrong.
#define new(a, n, t)    (t *)alloc(a, n, sizeof(t), _Alignof(t))

// Element count of a true array (never a pointer), as a signed size.
#define countof(a)      ((ptrdiff_t)(sizeof(a) / sizeof(*(a))))

// String literal -> Str, length computed at compile time, no strlen.
#define S(s)            (Str){s, sizeof(s)-1}


// -------------------------------------------------------------------
// Arena: bump allocator
// -------------------------------------------------------------------

// The whole allocator: two pointers. beg is the bump cursor, end is the
// hard limit. Passing an Arena *by value* gives a scratch arena, whose
// allocations vanish when the copy goes out of scope.
typedef struct {
    char *beg;
    char *end;
} Arena;

// Bump `count` objects of `size` bytes off the front of the arena.
// Returns zeroed memory, so every structure in this file is usable in
// its all-bits-zero state. There is no free(); the arena copy is the
// lifetime.
void *alloc(Arena *a, ptrdiff_t count, ptrdiff_t size, ptrdiff_t align)
{
    // Bytes needed to round beg up to `align` (align is a power of two).
    ptrdiff_t pad = -(uintptr_t)a->beg & (align - 1);
    // Division, not multiplication, so the check itself cannot overflow.
    assert(count < (a->end - a->beg - pad)/size);  // TODO: OOM policy
    void *r = a->beg + pad;
    a->beg += pad + count*size;
    return memset(r, 0, count*size);
}


// -------------------------------------------------------------------
// Str: counted string (a std::string_view equivalent)
// -------------------------------------------------------------------

// Pointer + length, never null-terminated. Ownership is not implied:
// data may point into a literal, into an arena, or nowhere at all when
// the Str is zero.
typedef struct {
    char     *data;
    ptrdiff_t len;
} Str;

// Duplicate s into the arena. The `if (r.len)` guard exists because
// memcpy forbids null pointers even for a zero count, and s may be a
// zero-initialised Str.
Str copy(Arena *a, Str s)
{
    Str r = s;
    r.data = new(a, s.len, char);
    if (r.len) memcpy(r.data, s.data, r.len);
    return r;
}

// head + tail, in place when head is already the most recent thing in
// the arena — then tail's copy lands immediately after it and only the
// length has to grow. Otherwise head is relocated to the bump pointer
// first. This is what makes repeated concat O(total length).
Str concat(Arena *a, Str head, Str tail)
{
    if (!head.data || head.data+head.len != a->beg) {
        head = copy(a, head);
    }
    head.len += copy(a, tail).len;
    return head;
}

// Byte equality. The `!a.len` short-circuit is the same memcmp
// null-pointer dodge as in copy() above.
_Bool equals(Str a, Str b)
{
    if (a.len != b.len) {
        return 0;
    }
    return !a.len || !memcmp(a.data, b.data, a.len);
}

// FNV-style multiplicative hash. The 0x100 basis stops runs of NUL
// bytes collapsing to zero; the multiplier is a large prime. Masking
// with 255 keeps the result independent of whether char is signed.
// High bits are the well-mixed ones — both tables below consume the
// hash from the top.
uint64_t hash64(Str s)
{
    uint64_t h = 0x100;
    for (ptrdiff_t i = 0; i < s.len; i++) {
        h ^= s.data[i] & 255;
        h *= 1111111111111111111;
    }
    return h;
}

// printf straight into the arena, returning the formatted text as a Str
// with no intermediate buffer. Only the bytes actually written are
// committed; the NUL vsnprintf appends is left uncommitted past beg.
Str print(Arena *a, char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    ptrdiff_t cap = a->end - a->beg;
    ptrdiff_t len = vsnprintf(a->beg, cap, fmt, ap);
    va_end(ap);

    Str r = {0};
    if (len<0 || len>=cap) {
        return r;  // TODO: trigger OOM
    }
    r.data  = a->beg;
    r.len   = len;
    a->beg += r.len;
    return r;
}


// -------------------------------------------------------------------
// Slice (push): dynamic array, grown through a pointer-returning macro
// -------------------------------------------------------------------

// Reserve one slot at the end of slice `s` and evaluate to a pointer to
// it, growing the backing store first if the slice is full. Works on
// any struct with data/len/cap fields — no templates needed, because
// push_() only ever touches the header generically.
//
// Evalutes S many times and A possibly zero times.
#define push(a, s) \
    ((s)->len == (s)->cap \
        ? (s)->data = push_((a), (s)->data, &(s)->cap, sizeof(*(s)->data)), \
          (s)->data + (s)->len++ \
        : (s)->data + (s)->len++)

// Growth helper for push(). Same in-place-if-possible trick as concat:
// if the array already sits at the bump pointer it is extended where it
// lies, otherwise it is relocated first. Capacity doubles (from 4).
// Slice data is only pointer-aligned, since standard C forbids
// _Alignof on an expression.
void *push_(Arena *a, void *data, ptrdiff_t *pcap, ptrdiff_t size)
{
    ptrdiff_t cap   = *pcap;
    ptrdiff_t align = _Alignof(void *);

    if (!data || a->beg != (char *)data + cap*size) {
        void *copy = alloc(a, cap, size, align);  // copy to bump pointer
        if (data) memcpy(copy, data, cap*size);
        data = copy;
    }

    ptrdiff_t extend = cap ? cap : 4;
    alloc(a, extend, size, 1);  // grow the backing buffer
    *pcap = cap + extend;
    return data;
}

// One slice type per element type. Zero-initialised is a valid empty
// slice — no constructor, no reserve call.
typedef struct {
    Str      *data;
    ptrdiff_t len;
    ptrdiff_t cap;
} StrSlice;

// 256 pushes into an empty slice; takes the arena by value, so all of
// it — words, strings and every abandoned intermediate buffer — is
// released on return.
void push_demo(Arena scratch)
{
    StrSlice words = {0};
    for (int i = 0; i < 256; i++) {
        Str word = print(&scratch, "word%d", i);
        *push(&scratch, &words) = word;
    }

    Str element = words.data[100];
    printf("%.*s\n", (int)element.len, element.data);
}


// -------------------------------------------------------------------
// Slice (append): the same idea without the macro
// -------------------------------------------------------------------

// Relocate a slice to the bump pointer, element by element.
StrSlice clone(Arena *a, StrSlice s)
{
    StrSlice r = {0};
    r.len = r.cap = s.len;
    r.data = new(a, s.len, Str);
    for (ptrdiff_t i = 0; i < s.len; i++) {
        r.data[i] = s.data[i];
    }
    return r;
}

// push() spelled as an ordinary, typed function: takes the slice by
// value and returns the updated header, so the caller must write
// `s = append(a, s, v)`. Costs one type-specific function per element
// type; buys readability and no multiple-evaluation hazard.
StrSlice append(Arena *a, StrSlice s, Str v)
{
    if (s.len == s.cap) {
        if (!s.data || (void *)(s.data + s.len) != a->beg) {
            s = clone(a, s);  // copy to bump pointer
        }
        ptrdiff_t extend = s.cap ? s.cap : 4;
        new(a, extend, Str);  // grow the backing buffer
        s.cap += extend;
    }
    s.data[s.len++] = v;
    return s;
}

// push_demo's twin, written against append().
void append_demo(Arena scratch)
{
    StrSlice words = {0};
    for (int i = 0; i < 256; i++) {
        Str word = print(&scratch, "word%d", i);
        words = append(&scratch, words, word);
    }

    Str element = words.data[100];
    printf("%.*s\n", (int)element.len, element.data);
}


// -------------------------------------------------------------------
// MSI: flat, fixed-capacity, open-address hash table
// -------------------------------------------------------------------

// Mask-Step-Index: two parallel fixed arrays, keys and values. A null
// key pointer marks an empty slot, so a zeroed FlatEnv is an empty
// table. Capacity is a hard ceiling — the table does not resize and
// does not detect overflow.
enum { ENVEXP = 10 };  // support up to 1,000 unique keys
typedef struct {
    Str keys[1<<ENVEXP];
    Str vals[1<<ENVEXP];
} FlatEnv;

// Lookup *and* insert in one function: returns the address of the value
// slot for `key`, claiming an empty slot if the key is absent. A null
// .data in the returned Str therefore means "was not present".
//
// Double hashing: the low bits index, the high bits (better mixed)
// supply an odd step, which is coprime with the power-of-two table size
// and so visits every slot. Overfilling the table spins forever.
Str *flatlookup(FlatEnv *env, Str key)
{
    uint64_t hash = hash64(key);
    uint32_t mask = (1<<ENVEXP) - 1;
    uint32_t step = (uint32_t)(hash>>(64 - ENVEXP)) | 1;
    for (int32_t i = (int32_t)hash;;) {
        i = (i + step) & mask;
        if (!env->keys[i].data) {
            env->keys[i] = key;
            return env->vals + i;
        } else if (equals(env->keys[i], key)) {
            return env->vals + i;
        }
    }
}

// Flatten the table into an execve(2)-style envp: an array of
// "KEY=VALUE\0" pointers. Iteration over a flat table is just a scan
// that skips empty slots. concat's in-place growth is what lets the
// three pieces become one contiguous C string.
char **flat_to_envp(FlatEnv *env, Arena *a)
{
    int    cap  = 1<<ENVEXP;
    char **envp = new(a, cap, char *);
    int len = 0;
    for (int i = 0; i < cap; i++) {
        if (env->vals[i].data) {
            Str pair = env->keys[i];
            pair = concat(a, pair, S("="));
            pair = concat(a, pair, env->vals[i]);
            pair = concat(a, pair, S("\0"));
            envp[len++] = pair.data;
        }
    }
    return envp;
}

// 256 key/value pairs into a 1024-slot table, then one lookup.
void msi_demo(Arena scratch)
{
    FlatEnv *env = new(&scratch, 1, FlatEnv);

    for (int i = 0; i < 256; i++) {
        Str key   = print(&scratch, "key%d", i);
        Str value = print(&scratch, "value%d", i);
        *flatlookup(env, key) = value;
    }

    Str value = *flatlookup(env, S("key100"));
    printf("%.*s\n", (int)value.len, value.data);
}


// -------------------------------------------------------------------
// Hash trie: unbounded map, no resize, no rehash
// -------------------------------------------------------------------

// 4-ary trie node. The map *is* the root pointer: a null Env * is an
// empty map. Nodes are never moved or rehashed, so every pointer handed
// out stays valid for the arena's lifetime.
typedef struct Env Env;
struct Env {
    Env *child[4];
    Str  key;
    Str  value;
};

// Lookup and insert, with the mode chosen by the arena argument:
// a null arena means pure lookup (returns 0 when absent), a real arena
// means insert-if-missing. Takes Env ** so the branch it walked can be
// written back into.
//
// Two hash bits are consumed per level, from the top down (h >>= 62,
// then h <<= 2). Depth is O(log4 n) for well-distributed keys.
Str *lookup(Env **env, Str key, Arena *a)
{
    for (uint64_t h = hash64(key); *env; h <<= 2) {
        if (equals(key, (*env)->key)) {
            return &(*env)->value;
        }
        env = &(*env)->child[h>>62];
    }
    if (!a) return 0;
    *env = new(a, 1, Env);
    (*env)->key = key;
    return &(*env)->value;
}

// Slice of C strings, for building envp out of the trie.
typedef struct {
    char    **data;
    ptrdiff_t len;
    ptrdiff_t cap;
} EnvpSlice;

// Recursive trie walk. Simple, but a lopsided trie (hostile keys, or
// just bad luck at ~100k entries) can exhaust the call stack — hence
// the iterative version below.
EnvpSlice env_to_envp_(EnvpSlice r, Env *env, Arena *a)
{
    if (env) {
        Str pair = env->key;
        pair = concat(a, pair, S("="));
        pair = concat(a, pair, env->value);
        pair = concat(a, pair, S("\0"));
        *push(a, &r) = pair.data;
        for (int i = 0; i < countof(env->child); i++) {
            r = env_to_envp_(r, env->child[i], a);
        }
    }
    return r;
}

// Caller-facing adapter over the recursive helper; the trailing push
// supplies the null terminator envp requires.
char **env_to_envp(Env *env, Arena *a)
{
    EnvpSlice r = {0};
    r = env_to_envp_(r, env, a);
    push(a, &r);
    return r.data;
}

// Same output, explicit stack instead of recursion, so depth costs
// arena rather than call frames.
//
// The trick worth noticing: `init` is a plain automatic array used as
// the stack's initial storage — a small-size optimisation that leaves
// no litter in the arena for the common case. push_() notices the
// stack no longer sits at the bump pointer and relocates it into the
// arena only if 16 frames prove insufficient.
char **env_to_envp_safe(Env *env, Arena *a)
{
    EnvpSlice r = {0};

    typedef struct {
        Env *env;
        int  index;
    } Frame;
    Frame init[16];  // small size optimization

    struct {
        Frame    *data;
        ptrdiff_t len;
        ptrdiff_t cap;
    } stack = {init, 0, countof(init)};

    *push(a, &stack) = (Frame){env, 0};
    while (stack.len) {
        Frame *top = stack.data + stack.len - 1;

        if (!top->env) {
            stack.len--;

        } else if (top->index == countof(top->env->child)) {
            // All four children visited: emit this node, then pop.
            Str pair = top->env->key;
            pair = concat(a, pair, S("="));
            pair = concat(a, pair, top->env->value);
            pair = concat(a, pair, S("\0"));
            *push(a, &r) = pair.data;
            stack.len--;

        } else {
            int i = top->index++;
            *push(a, &stack) = (Frame){top->env->child[i], 0};
        }
    }

    push(a, &r);
    return r.data;
}

// 256 inserts into a trie that starts as a null pointer, then one
// lookup with a null arena (lookup-only mode).
void hashtrie_demo(Arena scratch)
{
    Env *env = 0;

    for (int i = 0; i < 256; i++) {
        Str key   = print(&scratch, "key%d", i);
        Str value = print(&scratch, "value%d", i);
        *lookup(&env, key, &scratch) = value;
    }

    Str value = *lookup(&env, S("key100"), 0);
    printf("%.*s\n", (int)value.len, value.data);
}


// -------------------------------------------------------------------
// Test
// -------------------------------------------------------------------

// One 16 MiB block backs everything. Each demo receives the Arena by
// value, so all four start from the same empty arena and none can see
// another's allocations. Expected output: value100, value100, word100,
// word100. The block is never freed — process exit does that.
int main(void)
{
    int   cap = 1<<24;
    char *mem = malloc(cap);
    Arena a   = {mem, mem+(cap)};

    msi_demo(a);
    hashtrie_demo(a);
    push_demo(a);
    append_demo(a);
}
