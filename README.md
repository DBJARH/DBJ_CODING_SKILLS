# DBJ Coding Skills

A collection of small, focused C proof-of-concepts exploring coding
concepts and comparing approaches — currently centered on discriminated
unions / tagged records versus OOP-style dispatch.

## Contents

- [tribute_to_tony/](tribute_to_tony/) — a discriminated-union (tagged
  union) CRUD example in ISO C23, in the spirit of C.A.R. Hoare's 1966
  "Record Handling". For the concept itself — Hoare, Simula's
  `inspect`-shaped mistake, and why tagged unions resurfaced in
  Rust/Swift/Kotlin — see the published article:
  [iceberg.dbj.org/posts/tonyhoare](https://iceberg.dbj.org/posts/tonyhoare/).
  - [tribute_to_tony/dbj_email_crud.c](tribute_to_tony/dbj_email_crud.c) —
    the current POC, built on
    [dbj_email_record.h](tribute_to_tony/dbj_email_record.h),
    [dbj_email_storage_result.h](tribute_to_tony/dbj_email_storage_result.h)
    and [dbj_email_storage.h](tribute_to_tony/dbj_email_storage.h).
  - [tribute_to_tony/general_design.md](tribute_to_tony/general_design.md) —
    the design document for this POC.
  - [tribute_to_tony/top_level_requirements.md](tribute_to_tony/top_level_requirements.md) —
    requirements this POC implements (see RQ01).
  - [tribute_to_tony/analyzed_vibecode/](tribute_to_tony/analyzed_vibecode/) —
    design-discussion material only; nothing under this folder is meant
    to compile. Holds successive discarded AI-generated drafts
    (`v0_initial_claude_code_vibes/`, `v1_substandard_claude_design/`)
    each paired with a review/discussion file, kept for comparison.

## Building

See [build.md](build.md).

## License

MIT — see [LICENSE](LICENSE).
