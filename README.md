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

- [ken_thompson_grep/](ken_thompson_grep/) — the same tagged-union
  exercise applied to code that predates the idea: a C23 port of the
  regular expression engine from Ken Thompson's Version 6 Unix `grep`
  (c. 1975). The original encoded its opcodes as untyped bytes packed
  into one flat `char expbuf[512]`, where each opcode had a different
  operand width and nothing in the type system recorded that; the port
  makes the instruction stream an explicit tagged union over an
  exhaustive `switch`.
  - [ken_thompson_grep/dbj_grep.h](ken_thompson_grep/dbj_grep.h) —
    the engine: `DbjGrepInstruction`, `DbjGrepPattern`, `DbjGrepResult`.
  - [ken_thompson_grep/dbj_grep_test.c](ken_thompson_grep/dbj_grep_test.c) — its
    driver, with a built-in self test and a working grep mode.
  - [ken_thompson_grep/readme.md](ken_thompson_grep/readme.md) — folder
    readme and design document in one, diagrams included.
  - [ken_thompson_grep/ken_thompson_grep.md](ken_thompson_grep/ken_thompson_grep.md) —
    the legacy listing this port started from. **Not** authentic V6
    source despite claiming to be; an LLM reconstruction with the regex
    compiler missing entirely. Kept unfixed as the baseline, same
    reasoning as `analyzed_vibecode/` above. `readme.md` lists what is
    wrong with it.

## Building

See [build.md](build.md).

## License

MIT — see [LICENSE](LICENSE).
