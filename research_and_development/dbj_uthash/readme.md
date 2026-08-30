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

| ns per operation, 256 keys | uthash | Wellons | dbj ordinal | dbj string | dbj slice |
|---|---|---|---|---|---|
| find, present | 15.3 | 5.2 | 12.9 | 65.2 | 16.2 |
| find, absent | 8.7 | 6.8 | 12.5 | 54.9 | 17.1 |
| hash alone | 0.2 | 2.1 | — | 21.7 | 2.9 |
| insert, per key | 10.0 | — | 6.0 | — | — |

Everyone lands within a few nanoseconds except **dbj string at 65**. That one
hashes a fixed 32-byte buffer whatever the key's real length — six letters or
thirty, it hashes thirty-two. The slice key hashes only the letters that are
there, and costs 2.9 like everybody else.

Wellons is ahead on lookup because he returns a pointer into his own table
where dbj_hashmap copies the whole element out. uthash's 0.2 ns hash line is
`HASH_VALUE` on a six-byte literal with nothing stopping the optimiser folding
it, not a faster function.

See [../dbj_hashmap/readme.md](../dbj_hashmap/readme.md) for what those three
key kinds are.

## How trustworthy the numbers are

Every line above came out of
[../../third_party/dbj_ubenchtest](../../third_party/dbj_ubenchtest/) within
±1%, which is the point of that harness: it repeats each call until the sample
is well clear of the clock's own resolution, and refuses to report anything it
cannot resolve.

## Notes for whoever builds this

`-isystem src`, not `-I src` — uthash is someone else's code and does not
compile clean under this repo's `-Wall -Wextra -Werror`. `-isystem` silences its
warnings and leaves ours strict.

Each uthash call is wrapped in a small `static inline` function. That started
as a workaround — uthash's macros have commas in them, which broke the old
harness's block-as-one-macro-argument form — and stayed because it reads
better. Costs nothing at `-O2`.

Building against a different copy of uthash:

```
mingw32-make bench UTHASH_DIR=/some/where/uthash/src
```

---

(c) 2026 by dbj@dbj.org | MIT license
