---
version: 0.5
---

**Christopher Wellons ([skeeto](https://gist.github.com/skeeto)) for DBJ**

His techniques, kept here for comparison, and a dbj rework of the parts transforming and keeping for dbj perusal.

| File | What it is |
|---|---|
| [yet-another-good-corelib.md](yet-another-good-corelib.md) | The original article, reformatted. Unmodified prose. |
| [yet-another-good-corelib.c](yet-another-good-corelib.c) | The original runnable source, unmodified code, comments added. Reference only — do not develop here. |
| [dbj-arena-hashmap-hashtrie.c](dbj-arena-hashmap-hashtrie.c) | 0.1 — the dbj rework, keys are `dbj_str_slice` views into the arena. |
| [dbj_hash_string.h](dbj_hash_string.h) | 0.5 — the library types: `HashString`, `HashKey`, `HashMapElement`. |
| [dbj-str-4-welons.c](dbj-str-4-welons.c) | 0.5 — ordinal-keyed map over those types, plus the nanobench. |

Article: <https://nullprogram.com/blog/2025/01/19/> · gist: <https://gist.github.com/skeeto/42d8a23871642696b6b8de30d9222328> · upstream is public domain (Unlicense).

Why this is here: this repo currently has no arena allocator and no hash tables. Wellons has both, in a form that fits this repo's principles — data over domain, no OOP, zero-initialized values usable as-is.

Core principles this file must satisfy: [../CLAUDE.md](../CLAUDE.md).

## Release 0.1 plan

Agreed 2026-08-28, and implemented the same day. What follows is the record of what was decided and why, not a to-do list. Several of these decisions were revisited in 0.5 — see below.

### 0. Design diagram

Skipped by DBJ's call — the shapes are small enough to read straight off the source.

### 1. The string type — `dbj_str_slice`

Wellons' `Str` lands in the repo as [../toplevel/dbj_str_slice.h](../toplevel/dbj_str_slice.h), renamed `dbj_str_slice`.

It does **not** replace `DEFINE_DBJSTR_TYPE` in [../toplevel/dbj_str.h](../toplevel/dbj_str.h). The two are different kinds of a thing:

| | `str256` (dbj_str.h) | `dbj_str_slice` |
|---|---|---|
| owns its bytes | yes | no |
| size | the whole buffer, by value | 16 bytes |
| length field | none | yes |
| cost of a 4-char key | 256 bytes | 16 bytes + the 4 bytes it points at |

Which is the better default for this repo is **left open on purpose** — both exist now, and the comparison is the point. What is already clear is that the arena work below needs the borrowed one: `concat` grows a string in place only because the slice points *into* the arena, and the trie stores keys without copying them.

Key point is that (skeeto) `Str` has begin and end pointers pointing into the arena. DBJ str simply contains its own char array; it is not a slice aka "view", on external storage.

→ **Superseded in [0.5.1](#051-the-key-is-ordinal).** Not because either string type lost, but because the key stopped being a string at all. `dbj_str_slice` still backs 0.1; 0.5 keys are ordinal and its *values* are `dbj_str`.

### 2. C23, mandated

[../toplevel/dbj_required_compile_time.h](../toplevel/dbj_required_compile_time.h) first in the include list — GCC 15+, Clang rejected. `nullptr`, `bool`, `constexpr`, `[[maybe_unused]]` where they fit. Wellons did not mandate a standard; here it is not optional.

### 3. dbj naming and shape

Everything users will use from here gets a `dbj_` prefix and dbj shape: all-zero is a valid value, factory methods, user-defined types only, no OOP.

| Wellons | dbj |
|---|---|
| `Arena` | `dbj_arena` |
| `alloc` | `dbj_arena_alloc` |
| `new(a,n,t)` | `dbj_arena_new(a,n,t)` |
| `Str` | `dbj_str_slice` |
| `S(s)` | `DBJ_SS(s)` |
| `FlatEnv` | `dbj_hashmap` |
| `Env` | `dbj_hashtrie` |

The bare `new` / `S` / `push` macro names are dropped — they collide with everything in any real translation unit.

### 4. `defer` on the one allocation

[../toplevel/dbj_defer.h](../toplevel/dbj_defer.h) on the single `malloc` backing the arena in `main`. That is the only allocation in the program; everything else is bump-allocated out of it and freed by scope, not by a call.

### 5. `dbj_clintro`

One [../toplevel/dbj_clintro.h](../toplevel/dbj_clintro.h) call at the top of `main`.

### 6. `DBJ_LOOP_AS` — tried, then dropped

[../toplevel/dbj_macros.h](../toplevel/dbj_macros.h) was used at first, then taken out. It is an opinionated dbj macro, not a mandatory one, and here it bought nothing: its counter is `size_t`, so both `clone` loops had to cast their `ptrdiff_t` bound going in, and the demo loops had to cast their counter coming out. Plain `for` needs neither.

### 7. C23 array parameters

CLAUDE.md rule 9: every array parameter uses the `static` + size-expression form, `main` included — `char *argv[static argc + 1]`. Most functions here take a pointer to a single object, not an array; those are unaffected.

### 8. Kept from Wellons

- the arena (bump allocator)
- the string view, with `copy` / `concat` / `equals` / `hash` / `print`
- the MSI flat hash map (fixed capacity, open addressing)
- the 4-ary hash trie (unbounded, no resize, no rehash)
- **one** slice mechanism — the typed, value-returning `append`

### 9. Dropped

- **the `envp` machinery** (`flat_to_envp`, `env_to_envp`, `env_to_envp_safe`, `EnvpSlice`) — POSIX `execve` scaffolding, dead weight on Windows, and not what this file is about.

- **the one `push` macro**: replaced with `append functions`. It evaluates its argument several times and reads badly. CLAUDE.md: no abstractions beyond what the context requires.

**Before** — Wellons' one macro, serving every `data`/`len`/`cap` struct there will ever be:

```c
// PROBLEM! Evaluates args S many times and arg A possibly zero times.
#define push(a, s) \
    ((s)->len == (s)->cap \
        ? (s)->data = push_((a), (s)->data, &(s)->cap, sizeof(*(s)->data)), \
          (s)->data + (s)->len++ \
        : (s)->data + (s)->len++)
```

**After** — one typed function per one type.  Both type are "slice like" thus handling pointers into the arena space.

>[!Note] Two for the str slice

```c
static dbj_strings
dbj_strings_append(dbj_arena *arena, dbj_strings array, dbj_str_slice value);

static dbj_strings
dbj_strings_clone(dbj_arena *arena, dbj_strings source);
```
>[!Note] Two for the hashtrie stack

```c
static dbj_hashtrie_stack
dbj_hashtrie_stack_push(dbj_arena *arena, dbj_hashtrie_stack stack, dbj_hashtrie_frame frame);

static dbj_hashtrie_stack
dbj_hashtrie_stack_clone(dbj_arena *arena, dbj_hashtrie_stack source);
```

That is deliberate: two functions per type vs one "clever" macro for all types. Each `append` also carries a matching `clone`, so the real count is four functions against one macro, per one type


**`_Generic`** but also with C23, use `typeof` and other similar features

### 10. Added — seeded hash

The article's bonus section (ASLR-seeded hash) is folded in. Three lines, and it removes a real collision-attack surface. This is the one place the rework deliberately differs from upstream rather than merely renaming it.

→ **Kept, differently, in [0.5.5](#055-ordinal-keys-need-mixing).** The map's address still seeds the mix. What changed is what is being mixed: an ordinal key, not text — so the FNV string loop gives way to splitmix64's finaliser.

### 11. Not used yet — `dbj_result.h`

[../toplevel/dbj_result.h](../toplevel/dbj_result.h) carries two 512-byte char arrays per error. Right at a CRUD app's boundary; wrong inside a hot `lookup`. Deferred until after release 1.0. A comment marks the OOM `assert` as the place it would go.

→ **Reversed in [0.5.3](#053-result-idiom-everywhere-no-asserts).** The size objection was real but the conclusion was wrong: what made the result type fit was splitting *absent* from *failed*. A missing key returns OK carrying an `HK_EMPTY` element, so the 1KB error arm is only ever built on genuine failure — which, in a fixed-capacity map, means never in normal running.

### 12. Not used — `ubenchtest`

[../third_party/ubenchtest/](../third_party/ubenchtest/) declares itself DEPRECATED in its own header; the active harness is [../toplevel/dbj_nanobench.h](../toplevel/dbj_nanobench.h).

Benchmarks do not go in this file either — nanobench wants its own translation unit, and this file stays a readable reference. A separate benchmark file comes later, if wanted.

→ **Half reversed in [0.5.6](#056-benchmarks-with-dbj_nanobench).** `ubenchtest` stays out. But "nanobench wants its own translation unit" was simply wrong — it needs `DBJ_NANOBENCH_IMPLEMENTATION` defined once, which a single-file program satisfies. The benchmarks are in [dbj-str-4-welons.c](dbj-str-4-welons.c) itself.

### 13. Build

[Makefile](Makefile) follows the repo pattern, output under `$DBJ_BUILDS` or `../builds` as `dbj_arena_hashmap_hashtrie`. See [../build.md](../build.md). F5 in VS Code builds and debugs the active file independently of `make`.

`-std=gnu23`, not the `-std=c2x` used elsewhere in the repo: `dbj_defer.h` is built on GNU nested functions, which strict ISO mode rejects.

`dbj_arena_print` carries no `[[gnu::format]]` attribute. An earlier draft had one, to satisfy `%zu` under MinGW's msvcrt-flavoured format checking — but `%zu` was never required here. Plain `%d` with a cast at the call site says the same thing and needs no attribute to defend it.

## Release 0.5

Agreed 2026-08-28. Where it overrides 0.1, it says so.

### 0.5.1 The key is ordinal

Wellons' maps are string-to-string, so a key must be stored and compared. Here `KeyType` (an `unsigned int` by default, `#define`-able by the application) *is* the hash input: no string to store, none to compare, and no collision possible, because the key is itself rather than a digest of itself.

This is the largest departure from the article so far. The 0.1 file remains string-keyed, so the two sit side by side.

### 0.5.2 Three types, all discriminated unions

In [dbj_hash_string.h](dbj_hash_string.h), a library header — nothing in it knows about a particular map or application.

```c
HashString      the value: any one dbj_str size, tagged by type_id.
                A pure value type. It is never "empty" or "null".

HashKey         the key, and with it the state of the slot:
                HK_EMPTY, HK_NULL, HK_VALUE.

HashMapElement  one slot: { HashKey key; HashString val; }
```

The three key states are the three a single SQL cell has — empty, null, value. All slot behaviour lives on the key; the value stays a value.

What that buys: absence is no longer inferred from all-zero bytes, so **any** value at all is legal to store. In 0.1 an empty string could not be a key, because empty *was* the sentinel.

### 0.5.3 Result idiom everywhere, no asserts

`DBJ_MAKERESULT(HashMapElement)` from [../toplevel/dbj_result.h](../toplevel/dbj_result.h), used unchanged — this reverses §11 of the 0.1 plan.

The split that makes it work:

- **not found is not an error.** A missing key returns `OK`, carrying an element whose `key.id` is `HK_EMPTY`. Absence is an answer.
- **`ERR` is for the map being unable to answer** — capacity exhausted, and nothing else.

`dbj_arena_alloc` likewise returns `nullptr` rather than asserting. No `assert` remains in either file's map path.

### 0.5.4 No shift idiom

`#define DBJ_HASHMAP_SLOTS 1024`, not `1 << 10`. The shift said nothing the number does not. `DBJ_HASHMAP_MASK` and `DBJ_HASHMAP_EXP` follow from it and are stated once.

### 0.5.5 Ordinal keys need mixing

Consecutive keys (0, 1, 2 …) hash to consecutive slots, which is fine for the index but leaves the probe *step* correlated. `dbj_hashmap_mix` is splitmix64's finaliser — cheap, and it makes the high bits, where the step comes from, as good as the low ones.

The probe stays bounded by the slot count. That bound is the loop's real termination condition: without it, a lookup in a *full* table — no match, no free slot — never returns, and that hits reads as well as writes.

`HK_NULL` does not stop a search. A null-valued entry is a *present* key, so probing continues past it exactly as past a value.

### 0.5.6 Benchmarks with `dbj_nanobench`

[../toplevel/dbj_nanobench.h](../toplevel/dbj_nanobench.h), following [../dbj_nanobench_test/smoketest.c](../dbj_nanobench_test/smoketest.c). This reverses §12 of the 0.1 plan — they are in the same translation unit after all, since nanobench only needs `DBJ_NANOBENCH_IMPLEMENTATION` defined once and this is a single-file program.

Four measurements: get-hit, get-miss, probe alone, and `HashString` construction for scale.

### 0.5.7 Still open

- **`ubenchtest`** stays unused — deprecated in its own header.
- **`dbj_str.h`'s `static sz` parameter.** `str32_create` demands a full 32 bytes, so no string literal can be passed; both this file and [../dbj_str_test/dbj_str_test.c](../dbj_str_test/dbj_str_test.c) work around it. Twice now. Not changed — toplevel is left alone — but worth a decision of its own.
- **Owned vs borrowed** remains undecided by design. 0.1 and 0.5 exist to be compared, not to crown a winner.

---

(c) 2026 by dbj@dbj.org | MIT license
