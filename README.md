# DBJ Coding Skills

Small, self-contained C23 proof-of-concepts. Each folder stands alone.

## The content

- [toplevel/](toplevel/) — shared headers used by the POCs: defer,
  result, logging, nanobench, string helpers, macros.
- [tribute_to_tony/](tribute_to_tony/) — tagged-union CRUD example,
  after Hoare's "Record Handling".
- [ken_thompson_grep/](ken_thompson_grep/) — C23 port of the regex
  engine from V6 Unix `grep`.
- [strassen_mat_mul/](strassen_mat_mul/) — Strassen matrix multiply,
  plus SoA vs AoS layout comparisons.
- [dbjobserve/](dbjobserve/) — job-board observer POC.
- [dbj_str_test/](dbj_str_test/) — tests for `dbj_str.h`.
- [dbj_nanobench_test/](dbj_nanobench_test/) — smoke test for
  `dbj_nanobench.h`.
- [dbj_the_game/](dbj_the_game/) — the integration POC: a C23 + raylib
  side-scroller that uses `toplevel/` and `third_party/` together, at
  size. See [why](#why-a-game-is-in-here).
- [third_party/](third_party/) — vendored libs: tau, dbc_assert,
  inifile, libcurl, ubenchtest.
- [builds/](builds/) — build output.

## Why a game is in here

Every other folder proves one idea, in one file, on its own terms.
That is what a POC is for, and it leaves one thing unproven: whether
the shared code in [toplevel/](toplevel/) and the vendored libraries in
[third_party/](third_party/) actually hold up when a program uses
several of them at once, over thousands of lines, against a real
external dependency.

`dbj_the_game/` is that proof. It is still a proof-of-concept — the
subject being proved is the toolkit, not the game. The game is only the
load it carries.

## Building

See [build.md](build.md).

## License

MIT — see [LICENSE](LICENSE).

---

(c) 2026 by dbj@dbj.org | MIT license
