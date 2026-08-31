---
version: 1.1
---

# research_and_development

The concept is stuff from under here goes to either corelib or deprecated.
What goes to corelib are headers developed and tested in here.
What goes to deprecated is whole folder from under here.

## corelib graduation records

Which header in [../corelib/](../corelib/) came from where. A header
with no originating POC was written straight into corelib; that is a
record, not a gap.

| ID | corelib header | comment |
|----|----------------|---------|
| 1 | `dbj_clintro.h` | No originating POC. Written for the CLI apps that wanted one banner call in `main()`. |
| 2 | `dbj_defer.h` | No originating POC. Jens Gustedt's `defer` macro, taken from his 2025-01-06 post and used repo-wide. |
| 3 | `dbj_macros.h` | No originating POC. `DBJ_LOOP` / `DBJ_LOOP_AS`, landed with `dbj_clintro.h`. |
| 4 | `dbj_nth_prime.h` | From the benchmarking work, `deprecated/dbj_nanobench/`. It is fixture load — a knowably slow computation to time — and `third_party/dbj_ubenchtest/` still uses it for that. |
| 5 | `dbj_required_compile_time.h` | No originating POC. The GCC 15+ / no-Clang gate, added when the repo settled on one compiler. |
| 6 | `dbj_result.h` | No originating POC. `DBJ_MAKERESULT` and the tagged-union result type, exercised first by `corelib_smoke_test.c`. |
| 7 | `dbj_simple_log.h` | No originating POC. Replaced an earlier `dbj_log.h`. |
| 8 | `dbj_str.h` | From [dbj_str_test/](dbj_str_test/), which is still its test folder. |
| 9 | `dbj_str_slice.h` | From [chris_welons/](chris_welons/). Ported from Wellons' `Str`; the borrowed-string complement to `dbj_str.h`. |

## Whats cooking at the moment

- [tribute_to_tony/](tribute_to_tony/) — tagged-union CRUD example,
  after Hoare's "Record Handling".
- [ken_thompson_grep/](ken_thompson_grep/) — C23 port of the regex
  engine from V6 Unix `grep`.
- [strassen_mat_mul/](strassen_mat_mul/) — Strassen matrix multiply,
  plus SoA vs AoS layout comparisons.
- [dbj_hashmap/](dbj_hashmap/) — fixed-slot hash map, three key kinds,
  a tag per slot instead of a magic empty value.
- [chris_welons/](chris_welons/) — Wellons' arena, hash trie and string
  slice, ported and measured against ours.
- [dbj_uthash/](dbj_uthash/) — uthash vendored and benchmarked, as the
  outside opinion on dbj_hashmap.
- [dbjobserve/](dbjobserve/) — job-board observer POC.
- [dbj_str_test/](dbj_str_test/) — tests for `dbj_str.h`.
- [dbj_the_game/](dbj_the_game/) — the integration POC: a C23 + raylib
  side-scroller that uses `corelib/` and `third_party/` together, at
  size. See [why a game](../README.md#why-a-game-is-in-here).

## Building

Every folder here needs `DBJ_CORELIB` and `DBJ_BUILDS` set in the
environment — the first pointing at the repo's [corelib/](../corelib/),
the second at a build output folder. Neither has a fallback path: a
Makefile stops with an error if either is unset. See
[../build.md](../build.md).

Everything a folder here uses from outside itself lives one level up:
`$(DBJ_CORELIB)` for the shared headers, `../../third_party/` for
vendored libraries, and `$(DBJ_BUILDS)` for output.

---

(c) 2026 by dbj@dbj.org | MIT license
