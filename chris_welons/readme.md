---
version: 0.1
---

# chris_welons

Chris Wellons' (skeeto) core-lib techniques, kept here for comparison,
and a dbj rework of the parts worth keeping.

| File | What it is |
|---|---|
| [yet-another-good-corelib.md](yet-another-good-corelib.md) | The original article, reformatted. Unmodified prose. |
| [yet-another-good-corelib.c](yet-another-good-corelib.c) | The original runnable source, unmodified code, comments added. Reference only — do not develop here. |
| [dbj-arena-hashmap-hashtrie.c](dbj-arena-hashmap-hashtrie.c) | The dbj rework. This is the file under development. |

Article: <https://nullprogram.com/blog/2025/01/19/> · gist:
<https://gist.github.com/skeeto/42d8a23871642696b6b8de30d9222328> ·
upstream is public domain (Unlicense).

Why this is here: the repo has no arena allocator and no hash tables.
Wellons has both, in a form that fits this repo's principles — data over
domain, no OOP, zero-initialised values usable as-is.

Core principles this file must satisfy: [../CLAUDE.md](../CLAUDE.md).

## Plan

Agreed 2026-08-28, and implemented the same day — release 0.1.0. What
follows is the record of what was decided and why, not a to-do list.

### 0. Design diagram

Skipped by DBJ's call — the shapes are small enough to read straight
off the source.

### 1. The string type — `dbj_str_slice`

Wellons' `Str` lands in the repo as
[../toplevel/dbj_str_slice.h](../toplevel/dbj_str_slice.h), renamed
`dbj_str_slice`.

It does **not** replace `DEFINE_DBJSTR_TYPE` in
[../toplevel/dbj_str.h](../toplevel/dbj_str.h). The two are different
kinds of thing:

| | `str256` (dbj_str.h) | `dbj_str_slice` |
|---|---|---|
| owns its bytes | yes | no |
| size | the whole buffer, by value | 16 bytes |
| length field | none | yes |
| cost of a 4-char key | 256 bytes | 16 bytes + the 4 bytes it points at |

Which is the better default for this repo is **left open on purpose** —
both exist now, and the comparison is the point. What is already clear
is that the arena work below needs the borrowed one: `concat` grows a
string in place only because the slice points *into* the arena, and the
trie stores keys without copying them.

### 2. C23, mandated

[../toplevel/dbj_required_compile_time.h](../toplevel/dbj_required_compile_time.h)
first in the include list — GCC 15+, Clang rejected. `nullptr`, `bool`,
`constexpr`, `[[maybe_unused]]` where they fit. Wellons did not mandate
a standard; here it is not optional.

### 3. dbj naming and shape

Everything gets a `dbj_` prefix and dbj shape: all-zero is a valid
value, factory methods, user-defined types only, no OOP.

| Wellons | dbj |
|---|---|
| `Arena` | `dbj_arena` |
| `alloc` | `dbj_arena_alloc` |
| `new(a,n,t)` | `dbj_arena_new(a,n,t)` |
| `Str` | `dbj_str_slice` |
| `S(s)` | `DBJ_SS(s)` |
| `FlatEnv` | `dbj_hashmap` |
| `Env` | `dbj_hashtrie` |

The bare `new` / `S` / `push` macro names are dropped — they collide
with everything in any real translation unit.

### 4. `defer` on the one allocation

[../toplevel/dbj_defer.h](../toplevel/dbj_defer.h) on the single
`malloc` backing the arena in `main`. That is the only allocation in
the program; everything else is bump-allocated out of it and freed by
scope, not by a call.

### 5. `dbj_clintro`

One [../toplevel/dbj_clintro.h](../toplevel/dbj_clintro.h) call at the
top of `main`.

### 6. `DBJ_LOOP_AS`

[../toplevel/dbj_macros.h](../toplevel/dbj_macros.h) replaces the
hand-written `for` loops.

### 7. C23 array parameters

CLAUDE.md rule 9: every array parameter uses the `static` +
size-expression form, `main` included —
`char *argv[static argc + 1]`. Most functions here take a pointer to a
single object, not an array; those are unaffected.

### 8. Kept from Wellons

- the arena (bump allocator)
- the string view, with `copy` / `concat` / `equals` / `hash` / `print`
- the MSI flat hash map (fixed capacity, open addressing)
- the 4-ary hash trie (unbounded, no resize, no rehash)
- **one** slice mechanism — the typed, value-returning `append`

### 9. Dropped

- **the `envp` machinery** (`flat_to_envp`, `env_to_envp`,
  `env_to_envp_safe`, `EnvpSlice`) — POSIX `execve` scaffolding, dead
  weight on Windows, and not what this file is about.
- **the `push` macro**, in favour of `append`. It evaluates its
  argument several times and reads badly. CLAUDE.md: no abstractions
  beyond what the file demonstrates.

  The honest cost of that choice: `append` is typed, so the trie walk
  needs its own `dbj_hashtrie_stack_push` beside
  `dbj_str_slice_array_append` — two near-identical functions where the
  macro would have served both. That is the trade, taken deliberately:
  duplication that reads, over one clever macro that does not.

### 10. Added — seeded hash

The article's bonus section (ASLR-seeded hash) is folded in. Three
lines, and it removes a real collision-attack surface. This is the one
place the rework deliberately differs from upstream rather than merely
renaming it.

### 11. Not used yet — `dbj_result.h`

[../toplevel/dbj_result.h](../toplevel/dbj_result.h) carries two 512-byte
char arrays per error. Right at a CRUD app's boundary; wrong inside a
hot `lookup`. Deferred until after release 1.0. A comment marks the OOM
`assert` as the place it would go.

### 12. Not used — `ubenchtest`

[../third_party/ubenchtest/](../third_party/ubenchtest/) declares itself
DEPRECATED in its own header; the active harness is
[../toplevel/dbj_nanobench.h](../toplevel/dbj_nanobench.h).

Benchmarks do not go in this file either — nanobench wants its own
translation unit, and this file stays a readable reference. A separate
benchmark file comes later, if wanted.

### 13. Build

[Makefile](Makefile) follows the repo pattern, output under
`$DBJ_BUILDS` or `../builds` as `dbj_arena_hashmap_hashtrie`. See
[../build.md](../build.md). F5 in VS Code builds and debugs the active
file independently of `make`.

`-std=gnu23`, not the `-std=c2x` used elsewhere in the repo:
`dbj_defer.h` is built on GNU nested functions, which strict ISO mode
rejects.

One MinGW note, recorded because it costs an hour to rediscover:
`dbj_arena_print` is annotated `[[gnu::format(gnu_printf, 2, 3)]]`, not
`printf`. The plain `printf` archetype checks arguments against the old
msvcrt, which has no `%zu` or `%td`; this toolchain is UCRT, which has
both.

---

(c) 2026 by dbj@dbj.org | MIT license
