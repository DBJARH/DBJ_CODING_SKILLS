---
version: 1.1
---

# research_and_development

The concept is stuff from under here goes to either corelib or deprecated.
What goes to corelib are headers developed and tested in here.
What goes to deprecated is whole folder from under here.

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

Every folder here needs `DBJ_CORELIB` set in the environment, pointing
at the repo's [corelib/](../corelib/). There is no fallback path — a
Makefile stops with an error if it is unset. See
[../build.md](../build.md).

Everything a folder here uses from outside itself lives one level up:
`$(DBJ_CORELIB)` for the shared headers, `../../third_party/` for
vendored libraries, `../../builds/` for output when `DBJ_BUILDS` is
unset.

---

(c) 2026 by dbj@dbj.org | MIT license
