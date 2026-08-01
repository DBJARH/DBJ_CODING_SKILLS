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
- [third_party/](third_party/) — vendored libs: tau, dbc_assert,
  inifile, libcurl, ubenchtest.
- [builds/](builds/) — build output.

## Building

See [build.md](build.md).

## License

MIT — see [LICENSE](LICENSE).
