---
version: 0.1
---

# deprecated

Replaced, kept building where it stands. An archive does not empty:
things leave here by deletion or not at all.

Every folder under here is named in the table below. The reason for the
deprecation is not recorded and is not required — the fact that a folder
is here is the record.

| ID | folder | comment |
|----|--------|---------|
| 1 | [dbc_assert/](dbc_assert/) | QuantumLeaps Design By Contract for embedded C. Its readme is upstream text, left as it is. |
| 2 | [dbj_nanobench/](dbj_nanobench/) | Single-call benchmark timing. Superseded by [../third_party/dbj_ubenchtest/](../third_party/dbj_ubenchtest/), whose header points back at it. |
| 3 | [defer/](defer/) | The GCC nested-function `defer` polyfill, from `strassen_mat_mul/`. Superseded by [../corelib/dbj_defer.h](../corelib/dbj_defer.h), which its own header names. |

Both still build in place. See [../build.md](../build.md).

---

(c) 2026 by dbj@dbj.org | MIT license
