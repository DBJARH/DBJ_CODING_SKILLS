---
version: 0.2
---

# uthash

uthash by Troy D. Hanson, benchmarked against [dbj_hashmap](../dbj_hashmap/).

- https://troydhanson.github.io/uthash/
- `uthash.h` 2.4.0 sits in [src/](src/), unmodified.

## Run it

```
mingw32-make bench
```

## The numbers

256 keys, `-O2`, all measured in one session.

| 256 keys | uthash | dbj_hashmap |
|---|---|---|
| find a key that is there | 0.04 µs | 0.05 µs |
| find a key that is not | 0.03 µs | 0.05 µs |
| hash one short key | 0.03 µs | 0.03 µs |
| insert one key | 10 ns | 5 ns |

**uthash finds a bit faster. dbj_hashmap inserts twice as fast.**

## Why

The two tables are built differently.

**uthash grows.** Every key gets its own `malloc`. When it fills up it builds a
bigger table and moves everything across. It will take a hundred thousand keys.

**dbj_hashmap does not grow.** 1024 slots, one flat array, no `malloc` ever.
Key 1025 gets an error.

So the 2x on insert is not clever code. dbj_hashmap is faster because it does
less: never grows, never rehashes, never allocates. That is the same reason it
stops at 1024 keys and uthash does not.

### `malloc` is not the reason

Easy to assume uthash is slow because it allocates. It is not.

`malloc` costs about 0.30 µs per key — thirty times the insert itself. But it is
paid **once**, when the key goes in. A lookup allocates nothing, which is why
uthash finds just as fast as anyone.

The insert numbers above have allocation taken out of both sides, so they
compare like for like.

### Where chaining would actually cost

uthash puts each key in its own allocation, so a lookup hops around memory and
can miss the cache. A flat array does not.

At 256 keys the chains are too short for that to show. At 100,000 keys it would
— and dbj_hashmap would not be in the race at all, having stopped at 1024.

## How insert is measured

Fill the whole table with all 256 keys, time that, divide by 256. Allocation
happens before the clock starts.

Filling the *whole* table matters. Inserting one key into an empty table over
and over would measure the one case where uthash never has to grow, which
flatters it for the wrong reason.

## The full table

dbj_hashmap has three key kinds. Against uthash and Chris Wellons' hash trie:

| 256 keys | uthash | Wellons | dbj ordinal | dbj string | dbj slice |
|---|---|---|---|---|---|
| find, present (µs) | 0.04 | 0.03 | 0.05 | 0.12 | 0.05 |
| find, absent (µs) | 0.03 | 0.03 | 0.05 | 0.11 | 0.05 |
| hash alone (µs) | 0.03 | 0.03 | — | 0.08 | 0.03 |
| insert, per key (ns) | 10 | — | 5 | — | — |

Everyone lands in the same place except **dbj string at 0.12**. That one hashes
a fixed 32-byte buffer whatever the key's real length — six letters or thirty,
it hashes thirty-two. The slice key hashes only the letters that are there, and
costs 0.03 like everybody else.

See [../dbj_hashmap/readme.md](../dbj_hashmap/readme.md) for what those three
key kinds are.

## Read the numbers loosely

`dbj_nanobench` reports `min=0.00` on most lines, so these averages sit near the
smallest gap its clock can see. The 2x and 4x differences are real. Anything
smaller is noise.

## Notes for whoever builds this

`-isystem src`, not `-I src` — uthash is someone else's code and does not
compile clean under this repo's `-Wall -Wextra -Werror`. `-isystem` silences its
warnings and leaves ours strict.

uthash's macros have commas in them. `DBJ_BENCH` takes its block as a single
macro argument, so the preprocessor reads those commas as argument separators
and the build fails. Each uthash call is wrapped in a small `static inline`
function to hide them. Costs nothing at `-O2`.

Building against a different copy of uthash:

```
mingw32-make bench UTHASH_DIR=/some/where/uthash/src
```

---

(c) 2026 by dbj@dbj.org | MIT license
