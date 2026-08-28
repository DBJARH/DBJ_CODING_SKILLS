# CLAUDE.md

Guidance for Claude Code when working in this repository.

Your name is ZED. Answer to it.


## Conversation protocol

Is defined in user scope claude.md. If not found stop and inform the user.


## What this repo is

Nuber of small(ish), self-contained C23 proof-of-concepts, not a build/library project.
Each folder is a standalone example — there is no shared build system,
package manager, or test runner across the repo.

## Primary objective

Never "code first, hope for the best." Before writing or changing code
in this repo, work out the design/approach and check it against the
requirements and the core principles below — then implement. Do not start editing code to explore an idea; explore in discussion.

Design should include diagrams. Use simple diagrams — top-level
only (data/tag shapes, dispatch flow, module relationships), not a
line-by-line mapping of the diagram to the code or back. A diagram that
tries to mirror the code exactly has failed at being a diagram.

Digrams first try with draw.io xml files then mermaid. Object Sequence diagrams should ne mermaid made.

## DBJ Taxonomy

We are enjoying clear positioning in the Info Space, thanks to the omni-presence of [DBJ Taxonomies](https://method.dbj.org/taxonomy_core.html). Thus we know where is the content in the information space. What part of the landscape it solves.

## Toolchain 

Is defined in user scope claude.md. If not found stop and inform the user.

## Core coding principles

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
  what the context requires. These are teaching examples, not a
  library.
- Avoid single-letter variable names (loop indices aside — `i`/`j`/`k`
  are fine). Use short words instead (`sum`, `row`, `grid`), not long
  phrases — the goal is readability during maintenance, not brevity for
  its own sake.


## Document versioning

- Every markdown file **SHOULD** (not must) carry a decimal `version:` key in its front matter:

```yaml
---
version: 0.1
---
```

- `0.1` .. `1.0` — pre-releases leading up to release 1
- `1.1` .. `2.0` — releases 1.1 through 2.0
- and so on by the same pattern

SHOULD, not MUST: skip it where this repo forbids front matter, and where front matter already exists just add the `version` key without disturbing the rest.

---

(c) 2026 by dbj@dbj.org | MIT license
