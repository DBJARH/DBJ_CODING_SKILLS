---
version: 0.8
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

## Capacity is a hard ceiling

`DBJ_HASHMAP_SLOTS` (default 1024, must be a power of two) does not grow. The probe is bounded by it, which is the loop's real termination condition: without that bound, a lookup in a *full* table — no match, no free slot — never returns, and that hits reads as well as writes. The smoke test covers it.

Keep the load well under capacity. Open addressing degrades as it fills, and past full it stops answering.

Note that the key union is as large as its largest member, so every slot pays for `KeyString` even in a map that only ever holds ordinals. That is the price of one map type over all key kinds.

## Measured

256 keys, `"key0".."key255"`, against [../chris_welons/wellons_benchmark.c](../chris_welons/wellons_benchmark.c) on the same data. Microseconds, `-O2`.

| | ordinal | owned string | slice | Wellons |
|---|---|---|---|---|
| get, key present | 0.05 | 0.12 | 0.05 | 0.03 |
| get, key absent | 0.05 | 0.11 | 0.05 | 0.03 |
| hash alone | — | 0.08 | 0.03 | 0.03 |

The slice hash matches Wellons' `hash64` exactly, which it should — both hash only the bytes present with the same FNV-style loop. The owned string's 0.08 is entirely its fixed 32-byte buffer: same map, same probe, only the key kind changed.

The remaining 0.05 against 0.03 on a full get is this map copying `HashMapElement` out by value, where Wellons returns a pointer into his table.

Read these as orders of magnitude, not measurements. `dbj_nanobench` reports `min=0.00` throughout, so the averages sit near its resolution floor — the 2-3x gaps are real, anything smaller is not.

## `-Wswitch-enum`

Every `switch` over a tag here has no `default`, on purpose, so an added enum value becomes a compile error. That needs `-Wswitch-enum`, not the `-Wswitch` that `-Wall` implies — `-Wswitch` falls silent as soon as a `default` exists, so it cannot enforce the rule. See [../CLAUDE.md](../CLAUDE.md).

This is what makes the key union extensible rather than merely open: adding `KT_SLICE` produced exactly two errors, both of them the arm that had to be written.

---

(c) 2026 by dbj@dbj.org | MIT license
