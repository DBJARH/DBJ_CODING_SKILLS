---
version: 1.0
---

# dbj_hashmap r&d

A [hash map](#vocabulary): you store a value under some key, and later you get it back by that key. This version is programed in C23 (latest standard of C). Lives entirely in headers, and is a r&d rather than a library to ship.

Two things make it perhaps unusual. 

1. **A key can be one of three different things.** Most maps take one kind of key — a string, or an integer. Here a key is a `HashKey`, a value carrying a small tag that says which kind it is. Adding a fourth kind means one new name in an enum and one new branch in each of two functions. The map itself does not change. See [tagged union](#vocabulary).
   1. Reason: hash key as a generic type. Example: if key is a number hashmap is fast, but if it is a word (aka string) it is flexible; think dictionary.
2. **"Not there" is an answer, not a magic value.** Many maps mark a free slot by storing a zero, or a null pointer, or `-1` — which means you can never store that **magical constant** value yourself. Here every slot carries its own state, separate from the value. 
   1. Think SQL and slots (aka cells) in a table. "Empty" table slot's are empty. Value can be NULL or some legal quantity. Have you ever questioned that?

## The files

| File | Description |
|---|---|
| [dbj_arena.h](dbj_arena.h) | A very simple way of handing out memory. You give it one big block up front, and it slices pieces off the front as you ask for them. You never free a single piece — you throw the whole block away at the end. |
| [dbj_hash_key.h](dbj_hash_key.h) | Everything to do with keys. It says what a key is allowed to be — a number, a short word, or a pointer to a word — and whether a slot is empty, holds a key with no value, or holds a key with a value. It also has the only two things the map ever asks of a key: turn yourself into a number, and compare yourself with another key. |
| [dbj_hash_string.h](dbj_hash_string.h) | The values you store. Each one is a fixed-size piece of text — 32, 64 or 128 bytes. Anything longer is cut off. |
| [dbj_hashmap_element.h](dbj_hashmap_element.h) | One key and one value, side by side. This is what a lookup hands back to you. It is not how the map keeps things internally. |
| [dbj_hashmap_element_result.h](dbj_hashmap_element_result.h) | What every operation actually returns: either an answer, or an explanation of why there is no answer. Also the short macros for asking which of the two you got, and for getting the answer out. |
| [dbj_hashmap.h](dbj_hashmap.h) | The map itself. Put a value in, get a value back, count what is in there. If you only want to use this thing, this is the one file to read. |
| [dbj_hashmap_smoketest.c](dbj_hashmap_smoketest.c) | 33 small tests. Every one of them is a bug the map really did have at some point. Change the map, run this. |
| [dbj_hashmap_benchmarks.c](dbj_hashmap_benchmarks.c) | Measures how long each operation takes, for all three kinds of key. Run it with `make bench`. |

## Use

```c
#define DBJ_MAKERESULT_IMPLEMENTATION   /* in exactly one .c file */
#include <dbj_hashmap.h>

static dbj_hashmap map;                 /* all zeros means empty; no setup call */

dbj_hashmap_set(&map, hash_key_ordinal(42), hash_string_32("forty two"));

HashMapElementResult found = dbj_hashmap_get(&map, hash_key_ordinal(42));
if (dbj_result_is_ok(found)) {
    switch (dbj_result_state(found)) {      /* HK_EMPTY | HK_NULL | HK_VALUE */
    case HK_VALUE: puts(hash_string_text(&dbj_result_value(found))); break;
    case HK_NULL:  /* the key is there, but has no value */ break;
    case HK_EMPTY: /* the key is not in the map */          break;
    }
}
```

Three things in that snippet are worth naming.

A `dbj_hashmap` full of zero bytes is a valid empty map. There is no constructor to call and no way to forget to call one.

`dbj_result_is_ok` asks a different question from "did you find it". It asks whether the map could answer at all. A key that simply is not in the map is a perfectly successful lookup — you get `OK`, and the state is `HK_EMPTY`. The only way to get `ERR` is a full map. More on that under [It never grows](#it-never-grows).

The one `#define` goes in exactly one `.c` file in your program. It is a convention borrowed from [../toplevel/dbj_result.h](../toplevel/dbj_result.h): that header declares a function everywhere and defines it once, and the `#define` picks which file gets the definition. See [translation unit](#vocabulary). Nothing else here needs it — everything else is [`static inline`](#vocabulary).

`make test` builds and runs the smoke test. `make bench` builds and runs the timings. The timings alone are built with `-O2`, because timing an unoptimised build measures something nobody ships.

## The key can be three things

| kind | how you make one | holds its own text? | length limit | what it costs |
|---|---|---|---|---|
| `KT_ORDINAL` | `hash_key_ordinal(n)` | n/a | — | nothing; the number is its own hash |
| `KT_STRING` | `hash_key_string("text")` | yes | 32 bytes | hashes all 32 bytes every time |
| `KT_SLICE` | `hash_key_slice(slice)` | no | none | hashes only the bytes that are there |

**Ordinal** — a plain number. It is its own [hash](#vocabulary), so there is nothing to compute and nothing to compare byte by byte. Two different numbers can never collide, because the key is itself rather than a summary of itself. This is the cheapest kind, and the floor everything else is measured against.

**Owned string** — the key carries its text with it, in a fixed 32-byte buffer. Anything longer is cut off. That has a consequence worth stating plainly: two keys that share their first 32 bytes are the same key.

**Slice** — a pointer and a length, 16 bytes, pointing at text stored somewhere else. Nothing is copied and there is no length limit. The catch is lifetime: the text must outlive the map. Point a slice at a string literal (those live forever), or copy the text into an [arena](#vocabulary):

```c
static char block[4096];
dbj_arena arena = dbj_arena_make(block, sizeof(block));

HashKey key = hash_key_slice_copy(&arena, some_transient_text);
```

Point a slice at a local buffer instead and the key turns to garbage the moment that function returns. Nothing in the code can catch that for you.

Keys of different kinds are never equal, whatever their bytes say. A `KT_STRING` and a `KT_SLICE` holding the same text are two different keys. That is deliberate: one owns its text and one borrows it, and quietly treating them as the same would hide the difference that matters.

### How the map stays ignorant of all this

The map never looks inside a key. It calls exactly two functions:

```c
uint64_t hash_key_hash(HashKey key);            /* turn a key into a number */
bool     hash_key_equal(HashKey lhs, HashKey rhs);  /* are these the same key? */
```

Both are a `switch` over the kind tag. Add a fourth kind and the compiler points at both of them and refuses to build until you have written the two new branches. That is the entire extension mechanism — see [Why no `default` case](#why-no-default-case).

## Three states, not two

A slot in the map is in one of three states:

- `HK_EMPTY` — nothing was ever put here.
- `HK_NULL` — a key is here, but it has no value.
- `HK_VALUE` — a key is here, with a value.

These are the three states a single cell in a SQL table has, and there will never be a fourth.

Two of these are usually rolled into one, and that is the bug this design avoids. If "empty" were signalled by the value being zero, you could never store a zero. Here the state lives beside the value instead of inside it, so every possible value is storable.

There are no `assert`s anywhere. A failure is returned, not shouted about. `dbj_arena_alloc` follows the same rule and returns `nullptr` when it runs out.

## It never grows

`DBJ_HASHMAP_SLOTS` — 1024 by default, and it must be a [power of two](#vocabulary) — is fixed at compile time. The map never reallocates. Past the last free slot, `dbj_hashmap_set` returns `ERR`.

This is not laziness, it is the one thing that makes the search terminate. Looking a key up means walking from slot to slot until you find the key or find an empty slot — see [probing](#vocabulary). In a *completely full* map there is neither, so without a hard bound on the walk, a lookup for a missing key spins forever. That is a hang on a *read*, which is the nastier half of it. The bound is the slot count. The smoke test covers it.

Practical advice: keep the map well under half full. Open addressing gets slower as it fills, and past full it stops answering.

One cost worth knowing. A `HashKey` is as big as its largest possible kind, so every slot pays for the 32-byte string key even in a map that only ever holds numbers. That is the price of having one map type that handles all key kinds.

## How it is laid out in memory

This section is about speed. Skip it if you only want to use the map — the behaviour is the same either way.

The obvious layout is one array of `HashMapElement`, each holding a key and a value together. That is what this map used to do, and it was slow for a reason that is invisible in the source.

A `HashMapElement` is 184 bytes, and 132 of those are the value. But looking a key up only ever reads the key part. So the search jumped 184 bytes at a time through 184 KB of memory to look at a few bytes of tag, and every jump dragged a value it did not care about into the [cache](#vocabulary) alongside it.

So the map now keeps three separate arrays instead:

```
dbj_hashmap
├── index[N]  { state, hash }   16 KB   the search walks only this
├── keys[N]   HashKey           40 KB   read only when the hash matched
└── vals[N]   HashString       132 KB   read only on an actual hit
```

Same total memory, same capacity. What changed is how much of it a search has to touch: 16 KB, which fits in the fastest cache and stays there.

Storing the hash next to the state is the other half of it. The hash is just a number, so a slot that is not the one you want is rejected by comparing two numbers, instead of by comparing 32 bytes of text. `hash_key_equal` still has the final say — two different keys *can* produce the same hash — but it now runs only on slots that already look promising.

`HashMapElement` still exists. It is what a lookup hands back to you. It is no longer how the map stores anything.

### One attribute that turned out to matter

The three operations are marked `[[gnu::always_inline]]`, and that is a measured decision rather than a preference.

Building an answer out of three arrays is more work for the compiler than copying one struct was. It was enough extra work that GCC decided `dbj_hashmap_set` was too big to [inline](#vocabulary) and gave it a real function call instead. A real call has to copy the 132-byte value argument onto the stack every single time. Insert went from 5 ns to 27 ns per key on that alone — a five-fold slowdown, caused by a compiler heuristic, from a change that touched none of the arithmetic.

The attribute puts it back. Worth remembering: for a header-only map like this one, the inliner is part of the design.

## Attributes, and why each one is there

Every map operation is `[[nodiscard]]` — the compiler warns if you throw the return value away. That is because the returned result is the *only* place the map reports being full. If you genuinely mean to ignore it, say so:

```c
(void)dbj_hashmap_set(&map, key, value);   /* deliberate, and it shows */
```

`dbj_hashmap_mix` is `[[gnu::const]]`, which promises the compiler that its answer depends on its arguments and on nothing else in the world — so two identical calls can be folded into one. Checked: they are.

`hash_key_hash` and `hash_key_equal` are `[[gnu::pure]]` instead, which is the weaker version of the same promise: same arguments give the same answer, as long as memory has not changed in between. They cannot be `const`, because a `KT_SLICE` key reads bytes that live elsewhere, and `const` promises reading no memory at all. The ordinal and string branches would each qualify as `const` on their own — but the attribute belongs to the whole function, so the weakest branch decides for all of them. That is what the two owning kinds pay for the borrowing kind existing.

`hash_key_empty`, `hash_string_128` and `hash_string_len` are `[[maybe_unused]]`. Nothing here calls them. They exist because the unions have those cases, and the attribute says so on purpose rather than leaving it looking like an oversight.

## Why no `default` case

Every `switch` over a tag in this code deliberately has no `default` branch, so that adding a new tag value becomes a compile error at every place that has to handle it.

This needs `-Wswitch-enum`, not the `-Wswitch` that `-Wall` gives you. `-Wswitch` goes quiet the moment a `default` exists, so it cannot enforce the rule. `-Wswitch-enum` names every unhandled value either way. See [../CLAUDE.md](../CLAUDE.md).

This is what makes the key a genuinely extensible design rather than merely an open-ended one. Adding `KT_SLICE` produced exactly two compile errors, and each one was a branch that had to be written.

## Measured

All numbers below: 256 keys, `"key0"` through `"key255"`, built `-O2`, on the same data throughout. See [ns and µs](#vocabulary) if the units are unfamiliar.

### What the optimisation pass moved

The repo's own timing tool measures one call at a time and cannot resolve anything below roughly 10 ns — which here is most of what we are trying to see. These numbers come from running each operation 10 million times and dividing.

| ns per operation | before | after |
|---|---|---|
| ordinal get, key present | 4.9 | **3.3** |
| ordinal get, key absent | 4.2 | **2.1** |
| slice get, key present | 7.8 | **6.4** |
| owned string get, key present | 65.7 | **41.3** |
| owned string hash alone | 32.9 | **2.6** |
| insert, warm | 9.7 | **7.1** |
| insert, cold fill | 9.7 | 9.9 |

The credit does not split evenly across the three changes.

The string hash getting 12x faster is one change on its own, and the reason is worth understanding. The old code hashed one byte at a time, and each step multiplied the running total from the step before. A CPU cannot start step two until step one has finished, so 32 bytes meant 32 multiplications strictly one after another. The new code takes 8 bytes at a time: four steps instead of thirty-two. Nothing clever, just fewer links in the chain. See [dependency chain](#vocabulary).

The rest of the gain is the three-array split.

**What did not move: cold fill.** Filling an empty map is no faster, and very slightly slower. Three arrays means an insert dirties three widely separated pieces of memory where one array dirtied three adjacent ones. That gives back roughly what the search saves. The honest summary: a map that is filled once and read many times wins here; a map that is constantly being refilled does not.

**What a string key still costs.** A full lookup is 41 ns while its own hash is 2.6 ns. Almost all of the remainder is the `HashKey` itself — 40 bytes — being copied by value through four functions in a row. That is the next thing to look at, and it is a question about the union, not about the map.

### Against other tables

Compared with [../chris_welons/wellons_benchmark.c](../chris_welons/wellons_benchmark.c) and [../dbj_uthash/uthash_benchmark.c](../dbj_uthash/uthash_benchmark.c), as `make bench` reports it. Note the units change per row.

| | ordinal | owned string | slice | Wellons | uthash |
|---|---|---|---|---|---|
| get, key present (µs) | 0.05 | 0.10 | 0.06 | 0.03 | 0.04 |
| get, key absent (µs) | 0.05 | 0.09 | 0.06 | 0.03 | 0.03 |
| hash alone (µs) | — | 0.05 | 0.03 | 0.03 | 0.03 |
| insert, per key (ns) | 5 | — | — | — | 10 |

Read this table as orders of magnitude only. At a 10 ns floor a 0.01 difference means nothing, and this table cannot see most of what the previous one measured.

The slice hash matches Wellons' `hash64`, as it should — both hash only the bytes present, by the same method. The owned string's extra cost is its fixed 32-byte buffer: same map, same search, only the key kind changed.

The remaining 0.05 against Wellons' 0.03 on a full lookup is this map copying the whole element out to you, where Wellons hands back a pointer into his own table.

The insert row is 2x against uthash, measured the same way in both — fill 256 keys, divide, allocation kept outside the timed part. It is **not** a claim that this map inserts better. uthash grows and rehashes as it fills and would happily accept a 100,000th key; this map has 1024 slots and says `ERR` past them. Refusing to grow is most of what that 2x buys. See [../dbj_uthash/readme.md](../dbj_uthash/readme.md).

## Vocabulary

**hash** — a function that turns a key into a number, and the number itself. The map uses that number to decide which slot to look in first. Good hashes spread different keys across different numbers.

**hash map** — a container mapping keys to values, where finding a key takes roughly the same time whether there are ten keys or ten thousand, because the hash points almost straight at the answer.

**tagged union** (also *discriminated union*) — a value that can be one of several different types, plus a small tag saying which one it currently is. Reading the wrong member is the classic bug; the tag is what prevents it.

**open addressing** — the collision strategy this map uses. When the slot a key hashes to is taken, the key goes into some other slot in the same array, found by a fixed rule. The alternative — a linked list hanging off each slot — is what uthash does.

**probing** — walking from slot to slot looking for a key, following that fixed rule. This map uses *double hashing*: the hash supplies both the starting slot and the step size to walk by. An odd step over a power-of-two sized array is guaranteed to visit every slot exactly once before repeating, which is what makes a bounded walk correct rather than merely cautious.

**power of two** — 2, 4, 8, 16 … 1024. Sizing the array this way lets "wrap around to the start" be a single bitwise AND instead of a division, which is many times slower.

**FNV-1a** — a small, well-known hash function: for each chunk of the input, XOR it into the running total, then multiply by a fixed prime. Simple and decent. Its weakness here was doing that one byte at a time.

**splitmix64** — the short sequence of shifts and multiplies this map runs over a hash before using it, to spread the bits out evenly. Cheap, and it makes the high bits (which supply the probe step) as well-mixed as the low ones.

**dependency chain** — a run of instructions where each needs the previous one's result, so the CPU cannot overlap them. Modern CPUs run several instructions at once when they are independent; a chain forbids that. Breaking a chain is often a bigger win than removing work.

**cache** — small, very fast memory the CPU keeps recently used data in. L1 is the smallest and fastest, tens of kilobytes. Data is moved in and out in fixed blocks (*cache lines*, typically 64 bytes), so touching one byte pulls in its 64 neighbours whether you wanted them or not. Layouts that keep the data you actually read close together are much faster than layouts that scatter it.

**inline** — the compiler pasting a function's body into the caller instead of emitting a call. It avoids the call itself and, more importantly, lets the compiler see through the function and delete work that turns out to be unnecessary. `[[gnu::always_inline]]` overrides the compiler's own judgement about when it is worth doing.

**static inline** — a function defined in a header, private to each file that includes it. It is how a header-only library ships code without a `.c` file and without duplicate-symbol errors at link time.

**translation unit** (TU) — one `.c` file plus everything it `#include`s: the unit the compiler processes at a time. "Define this in exactly one TU" means exactly one `.c` file in the whole program should produce the definition.

**bump allocator** / **arena** — the simplest possible allocator. It holds a block of memory and a pointer into it; allocating moves the pointer forward. There is no individual free — you throw the whole block away at once. Fast, and it makes lifetime a single obvious question.

**slice** — a pointer to some bytes plus a length. It refers to text without owning it, which is why it costs the same 16 bytes for any length, and why the text it points at has to outlive it.

**ns and µs** — nanosecond, a billionth of a second; microsecond, a millionth. 1 µs = 1000 ns. A modern CPU does a few instructions per nanosecond, and a trip to main memory costs a hundred or so.

---

(c) 2026 by dbj@dbj.org | MIT license
