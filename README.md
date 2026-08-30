# DBJ Coding Skills

Small, self-contained C23 proof-of-concepts. Each folder stands alone.

## The content

- [toplevel/](toplevel/) — shared headers used by the POCs: defer,
  result, logging, string helpers, macros.
- [tribute_to_tony/](tribute_to_tony/) — tagged-union CRUD example,
  after Hoare's "Record Handling".
- [ken_thompson_grep/](ken_thompson_grep/) — C23 port of the regex
  engine from V6 Unix `grep`.
- [strassen_mat_mul/](strassen_mat_mul/) — Strassen matrix multiply,
  plus SoA vs AoS layout comparisons.
- [dbjobserve/](dbjobserve/) — job-board observer POC.
- [dbj_str_test/](dbj_str_test/) — tests for `dbj_str.h`.
- [dbj_the_game/](dbj_the_game/) — the integration POC: a C23 + raylib
  side-scroller that uses `toplevel/` and `third_party/` together, at
  size. See [why](#why-a-game-is-in-here).
- [third_party/](third_party/) — vendored libs: tau, dbj_ubenchtest,
  dbc_assert, inifile, libcurl.
- [deprecated/](deprecated/) — replaced, kept building where it stands.
- [builds/](builds/) — build output.

## Testing and benchmarking

One choice each, for the whole repo.

- **Unit tests: [third_party/tau](third_party/tau/).** `TEST(suite,
  name)`, `CHECK_*`, `TAU_MAIN()`. Light and enough.
  `tau/dbj_tau_bench.h` is ours, not upstream: three stopwatch macros
  for timing something inside a `TEST`. It is not a benchmark harness.
- **Benchmarks: [third_party/dbj_ubenchtest](third_party/dbj_ubenchtest/).**
  Samples until the number is stable and reports a confidence
  interval, which is what a benchmark owes you. A benchmark body is
  one operation, however small: the harness works out how many calls
  it needs between two clock readings and divides back out, so an
  operation quicker than the clock's own tick still gets a real
  number.
- **Deprecated: `dbj_nanobench.h`**, now in
  [deprecated/dbj_nanobench/](deprecated/dbj_nanobench/). It timed one
  call at a time and could not resolve below roughly 10 ns.

The price of ubenchtest, so nobody rediscovers it: each `UBENCH()`
registers itself at startup and the run order is the linker's, not
yours. Benchmarks print shuffled. Accepted knowingly — the resolution
is worth more than the ordering.

Not a POC, but kept here for lack of a better home:

- [claude_permissions_almanah/](claude_permissions_almanah/) — reusable
  Claude Code permission profiles and settings, with instructions.

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
