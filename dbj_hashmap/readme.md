---
version: 0.9
---

# dbj_hashmap

A flat, fixed-capacity, open-addressing hash map over a **discriminated-union key**, and the value types it stores. Header only.

The key is a `HashKey`: ordinal, owned string, or slice. Adding a fourth kind is a case in one enum and an arm in each of two functions — the map itself does not change.

Decoupled out of [../chris_welons/](../chris_welons/), where it grew — see that folder's [readme](../chris_welons/readme.md) for how the design was arrived at.

| File | What it is |
|---|---|
| [dbj_arena.h](dbj_arena.h) | Bump allocator. Two pointers, no free. Not map-specific. |
| [dbj_hash_key.h](dbj_hash_key.h) | `HashKey` — the key, a union over key kinds. `HashKeyHandle` — the slot state. The two operations every kind must answer. |
| [dbj_hash_string.h](dbj_hash_string.h) | `HashString` — the value, a union over `dbj_str` sizes. |
| [dbj_hashmap_element.h](dbj_hashmap_element.h) | `HashMapElement` — one slot: a key handle and a value. |
| [dbj_hashmap_element_result.h](dbj_hashmap_element_result.h) | `HashMapElementResult` and the `dbj_result_*` accessor macros. |
| [dbj_hashmap.h](dbj_hashmap.h) | The map. |
| [dbj_hashmap_smoketest.c](dbj_hashmap_smoketest.c) | 33 checks, each one a case the map got wrong at some point. |
| [dbj_hashmap_benchmarks.c](dbj_hashmap_benchmarks.c) | `dbj_nanobench` measurements across all three key kinds. |

## Use

```c
#define DBJ_MAKERESULT_IMPLEMENTATION   /* in exactly one .c */
#include <dbj_hashmap.h>

static dbj_hashmap map;                 /* zeroed is empty; no constructor */

dbj_hashmap_set(&map, hash_key_ordinal(42), hash_string_32("forty two"));

HashMapElementResult found = dbj_hashmap_get(&map, hash_key_ordinal(42));
if (dbj_result_is_ok(found)) {
    switch (dbj_result_state(found)) {      /* HK_EMPTY | HK_NULL | HK_VALUE */
    case HK_VALUE: puts(hash_string_text(&dbj_result_value(found))); break;
    case HK_NULL:  /* key present, no value */ break;
    case HK_EMPTY: /* key not in the map */    break;
    }
}
```

`make test` builds and runs the smoke test; `make bench` the benchmarks. The benchmarks alone are built `-O2` — timing an unoptimised build measures something nobody ships.

Only that one `#define` is needed, and it is inherited from [../toplevel/dbj_result.h](../toplevel/dbj_result.h), whose factories are ordinary functions. Everything these headers add is `static inline`, so there is no STB-style implementation switch of their own — that split solves a problem this code does not have.

## The two axes

The key and the slot are separate questions, and they were once one confused type. They are now two:

```
HashKeyHandle
├── id:  HK_EMPTY | HK_NULL | HK_VALUE   the state of the slot
└── val: HashKey
         ├── kind: KT_ORDINAL | KT_STRING | KT_SLICE
         └── union { KeyOrdinal ord; KeyString str; dbj_str_slice slice; }
```

`HashKeyHandle`'s three states are the three a single SQL cell has, and they stay three. `HashKey`'s kinds are open-ended.

The map never reads a key's internals. It calls two functions:

```c
uint64_t hash_key_hash(HashKey key);
bool     hash_key_equal(HashKey lhs, HashKey rhs);
```

Both `switch` on `kind` with no `default`, so `-Wswitch-enum -Werror` names both sites when a kind is added. That is the whole extension mechanism.

## The three key kinds

| kind | factory | holds its text | ceiling | cost |
|---|---|---|---|---|
| `KT_ORDINAL` | `hash_key_ordinal(n)` | n/a | — | the key *is* its own hash |
| `KT_STRING` | `hash_key_string("text")` | yes | `KeyString` capacity (32) | hashes the whole fixed buffer |
| `KT_SLICE` | `hash_key_slice(slice)` | no | none | hashes only the bytes present |

**Ordinal** is the floor. `KeyOrdinal` is its own hash — nothing stored, nothing compared, no collision possible, because the key is itself rather than a digest of itself.

**Owned string** truncates at 32 bytes and hashes all 32 whatever the key's real length. Two keys sharing their first 32 bytes are one key.

**Slice** points at text it does not hold. No copy, no ceiling, any length for the same 16 bytes — but the text must outlive the map. Point it at a string literal, or use `hash_key_slice_copy` to put the text in an arena:

```c
static char block[4096];
dbj_arena arena = dbj_arena_make(block, sizeof(block));

HashKey key = hash_key_slice_copy(&arena, some_transient_text);
```

The arena answers the lifetime question. Point a slice at a stack buffer and the key is garbage the moment the frame goes; nothing in the code can check that for you.

Keys of different kinds are never equal. A `KT_STRING` and a `KT_SLICE` of the same text are two distinct keys — deliberately, because one holds its text and one does not, and equating them silently would hide that.

## Absence is a state of the slot

A slot's `HashKeyHandle` is `HK_EMPTY`, `HK_NULL` or `HK_VALUE` — the three states a single SQL cell has. Nothing infers "unused" from all-zero bytes, so **any** value at all is legal to store.

That is what lets the result idiom work cleanly:

- **A key that is not there returns `OK`**, carrying an element whose `key.id` is `HK_EMPTY`. Absence is an answer.
- **`ERR` means the map could not answer at all** — capacity exhausted, and nothing else.

There are no asserts. `dbj_arena_alloc` likewise returns `nullptr` rather than aborting.

## C23 attributes

Every map operation is `[[nodiscard]]`. The returned `HashMapElementResult` is the *only* report that the map was full, so dropping it discards the one signal that matters. A caller who genuinely means to must say so:

```c
(void)dbj_hashmap_set(&map, key, value);   /* deliberate, and it shows */
```

`dbj_hashmap_mix` is `[[gnu::const]]` — its result depends on nothing but its arguments, so GCC folds repeated calls. Verified: two identical calls compile to one.

`hash_key_hash` and `hash_key_equal` are `[[gnu::pure]]`, not `[[gnu::const]]`. `KT_SLICE` reads the bytes its pointer names, and `const` promises a function reads no memory at all. `pure` is the weaker promise that fits: same arguments, same result, provided memory has not changed in between. The ordinal and string arms would each qualify as `const` on their own — the attribute belongs to the function, so the weakest arm sets it for all of them. That is the price the two holding kinds pay for the pointing kind existing.

`hash_key_empty`, `hash_string_128` and `hash_string_len` are `[[maybe_unused]]`. Nothing in this library calls them; they exist because the unions have those cases, and the attribute says that is intended rather than an oversight.

## Storage is three arrays, not one

A `HashMapElement` is 184 bytes, 132 of them the value. Held in one
array, the probe strode 184 bytes through 184 KB to read four bytes of
tag, pulling a value into L1 at every step to ignore it.

```
dbj_hashmap
├── index[N]  { hk_id id; uint64_t hash; }   16 KB  the probe walks only this
├── keys[N]     HashKey                      40 KB  read on a hash match
└── vals[N]     HashString                  132 KB  read on a hit
```

Same bytes, same ceiling; what changed is how much of it a probe has to
touch. 16 KB stays in L1 at any load. The stored hash is the other half:
a mismatched slot now costs one 64-bit compare instead of a
`hash_key_equal` — a 32-byte `memcmp` for a string key, a dereference
for a slice. `hash_key_equal` still decides, on candidates only, because
a hash match is not a key match.

`HashMapElement` survives as the shape of an *answer*. It was the shape
of storage too until the probe was measured.

The three operations are `[[gnu::always_inline]]`, which is also
measured rather than preferred: assembling an element out of three
arrays costs enough inliner budget that GCC put `dbj_hashmap_set` out of
line, and an out-of-line `set` must copy its 132-byte `HashString`
argument onto the stack at every call site. That alone took insert from
5 ns to 27 ns per key.

## Capacity is a hard ceiling

`DBJ_HASHMAP_SLOTS` (default 1024, must be a power of two) does not grow. The probe is bounded by it, which is the loop's real termination condition: without that bound, a lookup in a *full* table — no match, no free slot — never returns, and that hits reads as well as writes. The smoke test covers it.

Keep the load well under capacity. Open addressing degrades as it fills, and past full it stops answering.

Note that the key union is as large as its largest member, so every slot pays for `KeyString` even in a map that only ever holds ordinals. That is the price of one map type over all key kinds. Splitting the arrays moved that cost out of the probe's path; it did not remove it.

## Measured

256 keys, `"key0".."key255"`, `-O2`, on the same data throughout.

### What the optimisation pass moved

`dbj_nanobench` times one call at a time and its floor is around 10 ns,
which here is most of the measurement. These are the same operations
timed over 10 million iterations and divided, which is the only way the
change is visible at all:

| ns/op | before | after |
|---|---|---|
| ordinal get, key present | 4.9 | **3.3** |
| ordinal get, key absent | 4.2 | **2.1** |
| slice get, key present | 7.8 | **6.4** |
| owned string get, key present | 65.7 | **41.3** |
| owned string hash alone | 32.9 | **2.6** |
| insert, warm | 9.7 | **7.1** |
| insert, cold fill | 9.7 | 9.9 |

Three changes, and the credit does not split evenly. The string hash's
12x is one change on its own: FNV-1a's multiply is a dependency chain,
so 32 bytes cost 32 back-to-back `imul`s, and latency was being paid
rather than throughput — four 8-byte rounds cost four. The rest is the
array split above.

Note what did *not* move: cold fill. Three arrays mean three distant
cache lines dirtied per insert where one array meant three adjacent
ones, and that pays back exactly what the probe saves. A fill-once
read-many map wins; a fill-heavy one does not.

Note also what a whole-string get still costs against its own hash: 41
against 2.6. The remainder is `HashKey` — 40 bytes — moving by value
through `hash_key_string`, `hash_key_hash`, `dbj_hashmap_slot` and
`hash_key_equal`. That is the next thing, and it is a question about
the union, not about the map.

### Against other tables

Against [../chris_welons/wellons_benchmark.c](../chris_welons/wellons_benchmark.c)
and [../dbj_uthash/uthash_benchmark.c](../dbj_uthash/uthash_benchmark.c),
as `make bench` reports it. Units per row. Read these as orders of
magnitude: at a 10 ns floor, a 0.01 difference is nothing, and the
table cannot see most of what the row above measured.

| | ordinal | owned string | slice | Wellons | uthash |
|---|---|---|---|---|---|
| get, key present (µs) | 0.05 | 0.10 | 0.06 | 0.03 | 0.04 |
| get, key absent (µs) | 0.05 | 0.09 | 0.06 | 0.03 | 0.03 |
| hash alone (µs) | — | 0.05 | 0.03 | 0.03 | 0.03 |
| insert, per key (ns) | 5 | — | — | — | 10 |

The slice hash matches Wellons' `hash64`, which it should — both hash only the bytes present with the same FNV-style loop. The owned string's remaining excess is its fixed 32-byte buffer: same map, same probe, only the key kind changed.

The remaining 0.05 against 0.03 on a full get is this map copying `HashMapElement` out by value, where Wellons returns a pointer into his table.

The insert row is 2x against uthash, measured the same way in both — a full 256-key fill, divided, allocation outside the timed region. It is not a claim that this map inserts better: uthash grows and rehashes as it fills and would take a 100,000th key, where this one has 1024 slots and answers `ERR` past them. Refusing to grow is most of what the 2x buys. See [../dbj_uthash/readme.md](../dbj_uthash/readme.md).

## `-Wswitch-enum`

Every `switch` over a tag here has no `default`, on purpose, so an added enum value becomes a compile error. That needs `-Wswitch-enum`, not the `-Wswitch` that `-Wall` implies — `-Wswitch` falls silent as soon as a `default` exists, so it cannot enforce the rule. See [../CLAUDE.md](../CLAUDE.md).

This is what makes the key union extensible rather than merely open: adding `KT_SLICE` produced exactly two errors, both of them the arm that had to be written.

---

(c) 2026 by dbj@dbj.org | MIT license
