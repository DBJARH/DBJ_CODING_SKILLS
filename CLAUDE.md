# CLAUDE.md

Guidance for Claude Code when working in this repository.

## What this repo is

Small, self-contained C proof-of-concepts, not a build/library project.
Each folder is a standalone example — there is no shared build system,
package manager, or test runner across the repo.

- `tribute_to_tony/` — the canonical, documented example (tagged unions /
  discriminated union dispatch, in the spirit of Hoare 1966). Treat
  `tribute_to_tony/readme.md` as the source of truth for the concept
  being demonstrated.
- `not_oop_dbj_improved/` — human-improved rewrite of the same example,
  deliberately avoiding OOP-style dispatch.
- `discarded_vibecode/` — the original AI-generated version, kept only
  as a before/after comparison. Do not "fix" or improve this folder; its
  value is being the unedited baseline.

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
9. Use C23 array parameters with `static` and size expressions on functions

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
