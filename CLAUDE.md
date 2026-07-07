# CLAUDE.md

Guidance for Claude Code when working in this repository.


> **Caveat Emptor**: We are enjoying the metapresence of [DBJ Taxonomies](https://method.dbj.org/taxonomy_core.html). Thus we know where are we in the information space with these endeavor. Top category: **Implementation**. Capability: **Development**. In other word: We know what is this all about. And we can explain it to the reader.
>


## What this repo is

Small, self-contained C proof-of-concepts, not a build/library project.
Each folder is a standalone example — there is no shared build system,
package manager, or test runner across the repo.

- `tribute_to_tony/` — the canonical, documented example (tagged unions /
  discriminated union dispatch, in the spirit of Hoare 1966). The
  concept itself (Hoare, Simula, OOP's dropped `inspect`, Rust's
  `enum`+`match`) is written up at
  https://iceberg.dbj.org/posts/tonyhoare/ — do not re-derive that essay
  in this repo's docs, link to it instead.
  - `tribute_to_tony/dbj_discriminated_union.c` is the current POC;
    `dbj_err.h`/`dbj_log.h` hold the shared `Result` type and logging
    macros it includes. `tribute_to_tony/dbj_discriminated_union.md`
    is the main design document for this POC — check it before
    changing the .c file, and keep it updated when the design changes.
  - `tribute_to_tony/top_level_requirements.md` is the source of truth
    for requirement IDs (e.g. RQ01) that other docs link to.
- `tribute_to_tony/analyzed_vibecode/` — design-discussion material
  only. Nothing under this folder is meant to compile — do not attempt
  to build or run any code here, and do not "fix" or improve it; its
  value is being the unedited baseline. Holds successive discarded
  drafts (`v0_initial_claude_code_vibes/`, `v1_substandard_claude_design/`),
  each paired with its own review/discussion file, kept for comparison.

## Primary objective

Never "code first, hope for the best." Before writing or changing code
in this repo, work out the design/approach and check it against the
requirements (`tribute_to_tony/top_level_requirements.md`) and the core
principles below — then implement. Do not start editing code to explore
an idea; explore in discussion or in `analyzed_vibecode/`-style
discardable drafts first.

Design must include diagrams. Use simple Mermaid diagrams — top-level
only (data/tag shapes, dispatch flow, module relationships), not a
line-by-line mapping of the diagram to the code or back. A diagram that
tries to mirror the code exactly has failed at being a diagram.

## DBJ Taxonomy

Design docs in this repo place themselves in the DBJ Taxonomy — see the
core definition: https://method.dbj.org/taxonomy_core.html. A design
doc should state its Top category and Capability up front (see
`tribute_to_tony/dbj_discriminated_union.md` for the pattern), so the
reader — human or AI — knows where the work sits in the information
space before reading further.

## Core principles

These apply across the whole repo. This is the single place they are
defined — do not restate or fork them in per-folder readmes; link back
here instead.

1. Only default constructors
2. Only factory methods
3. Only user-defined types
4. Exceptions: never
5. OOP: never
6. Data > Domain
7. Primary language: C23
8. Use `defer` statements wherever possible
9. Every array parameter, on every function including `main`, must use
   the C23 `static` + size-expression form — e.g.
   `char *argv[static argc + 1]`, not `char **argv` or `char argv[]`.
   Note the element type still has to satisfy the compiler's
   requirements for that function (`main`'s second parameter must stay
   compatible with `char **`, so it's `char *argv[static ...]`, an array
   of pointers, not `char argv[static ...]`, an array of `char`).

## Working conventions

- Code style: plain ISO C (C23 where used), no OOP idioms — no vtables,
  no `this`, no inheritance. The whole point of these examples is
  explicit tagged unions with an exhaustive `switch`.
- Never add a `default` case to a `switch` over an enum/tag in these
  examples — the missing `default` is intentional, so `-Wswitch -Werror`
  can catch unhandled enum variants at compile time.
- Keep functions in the "storage + params in, result out" shape used
  throughout — no hidden state, no methods on structs.
- Don't introduce abstractions, helper layers, or generalization beyond
  what a given file demonstrates. These are teaching examples, not a
  library.
