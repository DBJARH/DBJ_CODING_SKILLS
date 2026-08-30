# DBJ Coding Skills

Small, self-contained C23 proof-of-concepts. Each folder stands alone.

## The layout

- [corelib/](corelib/) — the shared headers: defer, result, logging,
  string helpers, macros. What earned its place, out of the R&D below.
- [research_and_development/](research_and_development/) — the POCs.
  Each one either graduates a header into `corelib/`, or the whole
  folder ends in `deprecated/`.
- [third_party/](third_party/) — vendored libs: tau, dbj_ubenchtest,
  inifile, libcurl, raylib.
- [deprecated/](deprecated/) — replaced, kept building where it stands.
- [builds/](builds/) — build output.

## Two environment variables

- **`DBJ_CORELIB` must point at this repo's `corelib/`.** Every
  Makefile stops with an error if it is unset. No fallback: a wrong
  corelib is a broken build, not a silent one.
- `DBJ_BUILDS`, optional, is where build output goes. Unset, each
  folder writes to `builds/` at the repo root.

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

## Why a game is in here

Every other folder proves one idea, in one file, on its own terms.
That is what a POC is for, and it leaves one thing unproven: whether
the shared code in [corelib/](corelib/) and the vendored libraries in
[third_party/](third_party/) actually hold up when a program uses
several of them at once, over thousands of lines, against a real
external dependency.

[research_and_development/dbj_the_game/](research_and_development/dbj_the_game/)
is that proof. It is still a proof-of-concept — the
subject being proved is the toolkit, not the game. The game is only the
load it carries.

## Building

See [build.md](build.md).

## License

MIT — see [LICENSE](LICENSE).

---

(c) 2026 by dbj@dbj.org | MIT license
