---
version: 0.5
---

# dbj_hashmap

A flat, fixed-capacity, open-addressing hash map with an **ordinal key**, and the value types it stores. Header only.

Decoupled out of [../chris_welons/](../chris_welons/), where it grew — see that folder's [readme](../chris_welons/readme.md) for how the design was arrived at, and for the string-keyed variant it is contrasted with.

| File | What it is |
|---|---|
| [dbj_arena.h](dbj_arena.h) | Bump allocator. Two pointers, no free. Not map-specific. |
| [dbj_hash_string.h](dbj_hash_string.h) | `HashString`, `HashKey`, `HashMapElement` — the stored types, all discriminated unions. |
| [dbj_hashmap_element_result.h](dbj_hashmap_element_result.h) | `HashMapElementResult` and the `dbj_result_*` accessor macros. |
| [dbj_hashmap.h](dbj_hashmap.h) | The map. |
| [dbj_hashmap_smoketest.c](dbj_hashmap_smoketest.c) | 18 checks, each one a case the map got wrong at some point. |

## Use

```c
#define DBJ_MAKERESULT_IMPLEMENTATION   /* in exactly one .c */
#include <dbj_hashmap.h>

static dbj_hashmap map;                 /* zeroed is empty; no constructor */

dbj_hashmap_set(&map, 42, hash_string_32("forty two"));

HashMapElementResult found = dbj_hashmap_get(&map, 42);
if (dbj_result_is_ok(found)) {
    switch (dbj_result_state(found)) {      /* HK_EMPTY | HK_NULL | HK_VALUE */
    case HK_VALUE: puts(hash_string_text(&dbj_result_value(found))); break;
    case HK_NULL:  /* key present, no value */ break;
    case HK_EMPTY: /* key not in the map */    break;
    }
}
```

`make test` builds and runs the smoke test.

Only that one `#define` is needed, and it is inherited from [../toplevel/dbj_result.h](../toplevel/dbj_result.h), whose factories are ordinary functions. Everything these headers add is `static inline`, so there is no STB-style implementation switch of their own — that split solves a problem this code does not have.

## Two things worth knowing before using it

**The key is ordinal, not text.** `KeyType` (default `unsigned int`, `#define`-able) *is* the hash input. Nothing stores a string and nothing compares one, and a collision cannot happen, because the key is itself rather than a digest of itself.

**Absence is a state of the slot, not a sentinel value.** A slot's `HashKey` is `HK_EMPTY`, `HK_NULL` or `HK_VALUE` — the three states a single SQL cell has. Nothing infers "unused" from all-zero bytes, so **any** value at all is legal to store.

That second point is what lets the result idiom work cleanly:

- **A key that is not there returns `OK`**, carrying an element whose `key.id` is `HK_EMPTY`. Absence is an answer.
- **`ERR` means the map could not answer at all** — capacity exhausted, and nothing else.

There are no asserts. `dbj_arena_alloc` likewise returns `nullptr` rather than aborting.

## C23 attributes

Every map operation is `[[nodiscard]]`. The returned `HashMapElementResult` is the *only* report that the map was full, so dropping it discards the one signal that matters. A caller who genuinely means to must say so:

```c
(void)dbj_hashmap_set(&map, key, value);   /* deliberate, and it shows */
```

`dbj_hashmap_mix` is `[[gnu::const]]` — its result depends on nothing but its arguments, so GCC folds repeated calls. Verified: two identical calls compile to one.

`hash_key_empty`, `hash_string_128` and `hash_string_len` are `[[maybe_unused]]`. Nothing in this library calls them; they exist because the union has those cases, and the attribute says that is intended rather than an oversight.

## Capacity is a hard ceiling

`DBJ_HASHMAP_SLOTS` (default 1024, must be a power of two) does not grow. The probe is bounded by it, which is the loop's real termination condition: without that bound, a lookup in a *full* table — no match, no free slot — never returns, and that hits reads as well as writes. The smoke test covers it.

Keep the load well under capacity. Open addressing degrades as it fills, and past full it stops answering.

## `-Wswitch-enum`

Every `switch` over a tag here has no `default`, on purpose, so an added enum value becomes a compile error. That needs `-Wswitch-enum`, not the `-Wswitch` that `-Wall` implies — `-Wswitch` falls silent as soon as a `default` exists, so it cannot enforce the rule. See [../CLAUDE.md](../CLAUDE.md).

---

(c) 2026 by dbj@dbj.org | MIT license
