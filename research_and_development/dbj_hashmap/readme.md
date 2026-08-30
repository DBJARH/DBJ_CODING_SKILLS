---
version: 1.4
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
| [dbj_arena.h](dbj_arena.h) | A very simple way of handing out memory. You give it one big block up front, and it slices pieces off the front as you ask for them. You never free a single piece — you throw the whole block away at the end, and when the block is used up it tells you so instead of guessing. |
| [dbj_hash_key.h](dbj_hash_key.h) | Everything to do with keys. It says what a key is allowed to be — a number, a short word, or a pointer to a word. It also has the only two things the map ever asks of a key: turn yourself into a number, and compare yourself with another key. |
| [dbj_hash_string.h](dbj_hash_string.h) | The values you store. Each one is a fixed-size piece of text — 32, 64 or 128 bytes. Anything longer is cut off. |
| [dbj_make_hashmap.h](dbj_make_hashmap.h) | The map machinery, for any key type and any value type. `DBJ_MAKE_HASHMAP(K, V)` writes out a whole map for that pair — the storage, the search, and what a lookup hands back. It knows nothing about keys or values in particular. |
| [dbj_hashmap.h](dbj_hashmap.h) | One line: `DBJ_MAKE_HASHMAP(HashKey, HashString)`. That is the map this folder is about — the two files above, chosen as the pair. |
| [dbj_hashmap_smoketest.c](dbj_hashmap_smoketest.c) | 33 small tests. Every one of them is a bug the map really did have at some point. Change the map, run this. |
| [dbj_make_hashmap_smoketest.c](dbj_make_hashmap_smoketest.c) | The same machinery with plain C types instead — a `short → bool` map and an `unsigned → double` map, in one file, sharing nothing. |
| [dbj_hashmap_benchmarks.c](dbj_hashmap_benchmarks.c) | Measures how long each operation takes, for all three kinds of key. Run it with `make bench`. |

## Use

```c
#define DBJ_MAKERESULT_IMPLEMENTATION   /* in exactly one .c file */
#include <dbj_hashmap.h>

/* the names are built from the two types the map was made of */
typedef hashmap_HashKey_HashString map_t;
#define ELEM DBJ_HASHMAP_ELEMENT_TYPE(HashKey, HashString)

static map_t map;                       /* all zeros means empty; no setup call */

hashmap_HashKey_HashString_set(&map, hash_key_ordinal(42), hash_string_32("forty two"));

hashmap_HashKey_HashString_elementResult found =
    hashmap_HashKey_HashString_get(&map, hash_key_ordinal(42));

if (dbj_result_is_ok(found)) {
    switch (dbj_hashmap_state(ELEM, found)) {
    case DBJ_SLOT_VALUE: puts(hash_string_text(&dbj_hashmap_value(ELEM, found))); break;
    case DBJ_SLOT_NULL:  /* the key is there, but has no value */ break;
    case DBJ_SLOT_EMPTY: /* the key is not in the map */          break;
    }
}
```

Three things in that snippet are worth naming.

1. A `dbj_hashmap` full of zero bytes is a valid empty map. There is no need for a  constructor to call and no way to forget to call one.

2. dbj_result_is_ok` answers  a different question from "did you find it".  A key that simply is not in the map is a perfectly successful lookup — you get `OK`, and the state is `DBJ_SLOT_EMPTY`. The only way to get `ERR` is a map overflow: when you try to inser beyond the given capacity. More on that under [It never grows](#it-never-grows).

3. The one `#define DBJ_MAKERESULT_IMPLEMENTATION` goes in exactly one `.c` file in your program.. See the [translation unit](#vocabulary). 


`make test` builds and runs the smoke test. `make bench` builds and runs the timings.

## This hash map key can be One of the three things

| kind | how you make one | holds its own text? | length limit | explanation |
|---|---|---|---|---|
| `KT_ORDINAL` | `hash_key_ordinal(n)` | n/a | — | **Ordinal** is a fancy name for a plain number. It is its own [hash](#vocabulary). The cheapest kind, and what every other kind is measured against. You are responsible for it being unique. |
| `KT_STRING` | `hash_key_string("text")` | yes | 32 bytes | **Owned string** — the key carries its text with it, in a fixed 32-byte array. Anything longer is cut off, so two keys sharing their first 32 bytes are the same key. Safe, as long as your keys are under 32 bytes. Hashes all 32 every time, whatever the real length. |
| `KT_SLICE` | `hash_key_slice(slice)` | no | none | **Slice** — a pointer and a length, 16 bytes, aimed at text stored somewhere else. Nothing is copied and there is no length limit, so it is faster than the owned string. The catch is lifetime: the text must outlive the map. |

A slice needs its text to live somewhere that will exist after map does not. An "arena" does that:

```c
// arena storage is in a static global variable
// it will leave as long as app lives
static char block[4096];
// single arena per application
dbj_arena arena = dbj_arena_make(block, sizeof(block));
// usage example -- an arena can run out, so this returns a result
HashKeyResult key = hash_key_slice_copy(&arena, some_transient_text);
if (dbj_result_is_ok(key)) {
    (void)hashmap_HashKey_HashString_set(&map, dbj_result_hash_key(key), some_value);
}
```

Point a slice at a local buffer instead and the key turns to garbage the moment that function returns. 


Keys of different kinds are never equal, whatever their bytes say. A `KT_STRING` and a `KT_SLICE` holding the same text are two different keys. That is deliberate.

### How the map stays ignorant of all this

The map never looks inside a key. It calls exactly two functions:

```c
uint64_t HashKey_hash(HashKey key);                /* turn a key into a number */
bool     HashKey_equal(HashKey lhs, HashKey rhs);  /* are these the same keys? */
```

The names are not a style choice. `DBJ_MAKE_HASHMAP(K, V)` builds them out of the key type's own name, so any type used as a key must supply `K_hash` and `K_equal`. There is no default and no shortcut for small types: no type is its own hash. The bits of a `short` are not a hash of that `short`, they *are* the `short`.

## Core logic: cell is in one of the possibe three states

A slot in the map is in one of three states:

- `DBJ_SLOT_EMPTY` — nothing was ever put here.
- `DBJ_SLOT_NULL` — a key is here, but it has no value.
- `DBJ_SLOT_VALUE` — a key is here, with a legal value.

The state belongs to the slot, not to the key and not to the value. It used to hang off the key, which was wrong in a way that only showed once the map became a macro: a key type has no business knowing it is in a table.

This is database logic: These are the three states, one of which, a single cell in a table can have.

Two of these are usually rolled into one, and that is the fault in thinking, this design avoids. If "empty" were signalled by the value being zero, you could never store a zero.   Leaving the "zero" or null to have a role in some application logic.






## It never grows

There are no `assert`s anywhere. A failure is returned, not shouted about — and it is returned in a form the caller can act on rather than merely detect. See [Failure is a value](#failure-is-a-value).



The value called `DBJ_HASHMAP_SLOTS` is 1024 by default, and it must be a [power of two](#vocabulary) ; it is fixed at compile time.  The consequence is the map never reallocates to self expand. That is a fast implementation with the trade of. If you try to use it, past the last free slot, the map's `_set` hands back a result in an error state — saying the map is full, and where — rather than a magic value you might mistake for an answer. See [Failure is a value](#failure-is-a-value).

This is not laziness, it is the one thing that makes the search terminate when it reached "the end". Looking a key up means walking from slot to slot until you find the key or find an empty slot — see [probing](#vocabulary).Or there are no more slots in a *completely full* map. 

Practical advice: keep the map well under half full. Addressing gets slower as it fills.

## Failure is a value

Anything that can fail returns a **result**. A result is in one of two shapes for two states:

- **OK** — it holds the thing you asked for.
- **ERR** — it holds why there is no thing.

First you ask which state it is in. What you read next depends on that answer.

The code is the part that matters. Each type that can fail names its own result kinds, in an enum called `<thing>_result_type`. Example for `dbj_arena`:

```c
typedef enum : unsigned short {
    DBJ_ARENA_ERR_NONE = 0,
    DBJ_ARENA_ERR_EXHAUSTED,
} dbj_arena_result_type;
```

So a caller tests a tag instead of matching on prose in a error message:

```c
dbj_arena_result br_ = dbj_arena_new(&arena, count, char);

// we know result is some kind of error
if (dbj_result_is_err(br_)) {
     // we know exactly what kind of error
    if (dbj_arena_result_type_of(br_) == DBJ_ARENA_ERR_EXHAUSTED) {
        /* ...fall back, or start a fresh arena, or give up cleanly... */
    }
}
```

Not every type gets rich result type. A type earns a `_result_type` when it has a failure worth caring for: `HashKey` has one because the arena underneath it can run out; `HashString` and the element type have none, because nothing about them can go wrong.



How it is laid out in memory

This section is about speed. Skip it if you only want to use the map.

The obvious layout is one array of elements, each holding a key and a value together. That is what this map used to do, and it was slow for a reason that is invisible in the source.

So the map now keeps three separate arrays instead:

```
hashmap_HashKey_HashString
├── index[N]  { state, hash }   16 KB   the search walks only this
├── keys[N]   HashKey           40 KB   read only when the hash matched
└── vals[N]   HashString       132 KB   read only on an actual hit
```

Same total memory, same capacity. What changed is how much of it a search has to touch: 16 KB, which fits in the fastest CPU cache and stays there.

Storing the hash next to the state is the other half of it. The hash is just a number, so a slot that is not the one you want is rejected by comparing two numbers, instead of by comparing 32 bytes of text. `HashKey_equal` still has the final say — two different keys *can* produce the same hash — but it now runs only on slots that already look promising.

The element type still exists. It is what a lookup hands back to you. It is no longer how the map stores anything.

### One attribute that turned out to matter

The three operations are marked `[[gnu::always_inline]]`, and that is a measured decision rather than a preference.

Building an answer out of three arrays is more work for the compiler than copying one struct was. It was enough extra work that GCC decided the map's `_set` was too big to [inline](#vocabulary) and gave it a real function call instead. A real call has to copy the 132-byte value argument onto the stack every single time. Insert went from 5 ns to 27 ns per key on that alone — a five-fold slowdown, caused by a compiler heuristic, from a change that touched none of the arithmetic.

The attribute puts it back. Worth remembering: for a header-only map like this one, the inliner is part of the design.

## Attributes, and why each one is there

Every map operation is `[[nodiscard]]` — the compiler warns if you throw the return value away. That is because the returned result is the *only* place the map reports being full. If you genuinely mean to ignore it, say so:

```c
(void)hashmap_HashKey_HashString_set(&map, key, value);   /* deliberate, and it shows */
```

`dbj_hashmap_mix` is `[[gnu::const]]`, which promises the compiler that its answer depends on its arguments and on nothing else in the world — so two identical calls can be folded into one. Checked: they are.

`HashKey_hash` and `HashKey_equal` are `[[gnu::pure]]` instead, which is the weaker version of the same promise: same arguments give the same answer, as long as memory has not changed in between. They cannot be `const`, because a `KT_SLICE` key reads bytes that live elsewhere, and `const` promises reading no memory at all. The ordinal and string branches would each qualify as `const` on their own — but the attribute belongs to the whole function, so the weakest branch decides for all of them. That is what the two owning kinds pay for the borrowing kind existing.

`hash_key_empty`, `hash_string_128` and `hash_string_len` are `[[maybe_unused]]`. Nothing here calls them. They exist because the unions have those cases, and the attribute says so on purpose rather than leaving it looking like an oversight.

## Why no `default` case

Every `switch` over a tag in this code deliberately has no `default` branch, so that adding a new tag value becomes a compile error at every place that has to handle it.

This needs `-Wswitch-enum`, not the `-Wswitch` that `-Wall` gives you. `-Wswitch` goes quiet the moment a `default` exists, so it cannot enforce the rule. `-Wswitch-enum` names every unhandled value either way. See [../CLAUDE.md](../CLAUDE.md).

This is what makes the key a genuinely extensible design rather than merely an open-ended one. Adding `KT_SLICE` produced exactly two compile errors, and each one was a branch that had to be written.

## Measured

All numbers below: 256 keys, `"key0"` through `"key255"`, built `-O2`, on the same data throughout. See [ns and µs](#vocabulary) if the units are unfamiliar.

### What the optimisation pass moved

`dbj_nanobench.h`, the repo's old timing tool (now in [../../deprecated/dbj_nanobench/](../../deprecated/dbj_nanobench/)), measured one call at a time and could not resolve anything below roughly 10 ns — which here is most of what we are trying to see. The numbers in this table came from running each operation 10 million times by hand and dividing.

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

### What `make bench` reports

Measured by [../third_party/dbj_ubenchtest](../third_party/dbj_ubenchtest/), which samples until the number stops moving and prints a confidence interval — every line below came in under ±0.7%.

Each benchmark body is a single operation. Every one of them is quicker than the clock can see one call at a time, so the harness runs the body thousands of times between two clock readings and divides back out; it prints the count it chose.

| ns per operation | ordinal | owned string | slice |
|---|---|---|---|
| get, key present | **12.9** | 65.2 | 16.2 |
| get, key absent | **12.5** | 54.9 | 17.1 |
| hash alone | — | 21.7 | 2.9 |
| probe only | 1.6 | — | — |
| insert, per key | 6.0 | — | — |

`hash_string_32`, building a value rather than looking one up, is 1.6 ns — the same as an ordinal probe.

The shape is the same story the table above tells: the slice hash is cheap because it hashes six bytes, the owned string hashes all 32 of its fixed buffer whatever the key's length, and the ordinal key is its own hash.

These numbers are **not** comparable to the previous table's. That one timed a bare operation in a hand-written loop; this one keeps the result alive across an optimisation barrier every iteration, and the result here is a whole element — key, state and a 132-byte value — copied out to the caller. The barrier is what makes the measurement honest, and it is also most of the difference.

### Against other tables

[../chris_welons/wellons_benchmark.c](../chris_welons/wellons_benchmark.c) and [../dbj_uthash/uthash_benchmark.c](../dbj_uthash/uthash_benchmark.c) now measure the same way, on the same harness, over the same 256 keys — so the three sit in one table at last.

| ns per operation | ordinal | owned string | slice | Wellons hashtrie | uthash |
|---|---|---|---|---|---|
| get, key present | **12.9** | 65.2 | 16.2 | 5.2 | 15.3 |
| get, key absent | **12.5** | 54.9 | 17.1 | 6.8 | 8.7 |
| hash alone | — | 21.7 | 2.9 | 2.1 | 0.2 |
| insert, per key | 6.0 | — | — | — | 10.0 |

The slice hash is 2.9 ns against Wellons' 2.1, as it should be — both hash only the bytes present, by the same method. The owned string's 21.7 is its fixed 32-byte buffer: same map, same search, only the key kind changed.

Wellons' hashtrie beats this map on lookup, 5.2 against 12.9, and the reason is in the return: he hands back a pointer into his own table, where this map copies the whole element — key, state and a 132-byte value — out to the caller. That copy is most of the difference and it is a deliberate choice, not an oversight.

uthash's hash line, 0.2 ns, is not a faster hash. It is `HASH_VALUE` on a 6-byte key with nothing to stop the optimiser folding it, where the other three carry their key through a `HashKey` union first.

The insert row is measured the way uthash's own benchmark measures it — fill 256 keys, divide, allocation kept outside the timed part — and it keeps its own clock, because a benchmark that must empty the map between runs cannot let that clearing be timed with the inserts. It is **not** a claim that this map inserts better. uthash grows and rehashes as it fills and would happily accept a 100,000th key; this map has 1024 slots and says `ERR` past them. Refusing to grow is most of what the difference buys. See [../dbj_uthash/readme.md](../dbj_uthash/readme.md).

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

**bump allocator** / **arena** — the simplest possible allocator. It holds a block of memory and a pointer into it; allocating moves the pointer forward. There is no individual free — you throw the whole block away at once. Fast, and it makes lifetime a single obvious question. It can of course run out, which it reports; see [Failure is a value](#failure-is-a-value).

**result** — a returned value that is either the thing you asked for or the reason you cannot have it, never both and never neither. It is how this code reports failure without aborting and without reserving a magic value. Reading one means asking `dbj_result_is_ok` first; everything else depends on the answer.

**slice** — a pointer to some bytes plus a length. It refers to text without owning it, which is why it costs the same 16 bytes for any length, and why the text it points at has to outlive it.

**ns and µs** — nanosecond, a billionth of a second; microsecond, a millionth. 1 µs = 1000 ns. A modern CPU does a few instructions per nanosecond, and a trip to main memory costs a hundred or so.

---

(c) 2026 by dbj@dbj.org | MIT license
