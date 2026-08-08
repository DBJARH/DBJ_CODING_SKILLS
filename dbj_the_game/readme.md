---
version: 0.1
---

# dbj_the_game

A C23 + raylib rewrite of a C++14/SFML side-scroller.
Design: [docs/design.md](docs/design.md).

## Why

What the POCs (aka "small examples") elsewhere in this repo cannot show is whether
[toplevel/](../toplevel/) and [third_party/](../third_party/) survive
contact with a real program.

## What is used 

These artefacts are to be used. Where they land in the code is decided 32while writing it.

<br id="artefacts_used">

| Artefact | Has to show |
|---|---|
| `toplevel/dbj_defer.h` | resource ownership that survives early returns |
| `toplevel/dbj_result.h` | failure paths that are not `exit()` |
| `toplevel/dbj_simple_log.h` | logging usable inside a frame loop |
| `toplevel/dbj_clintro.h` | startup banner |
| `toplevel/dbj_macros.h` | loop macros over real data, not toy arrays |
| `toplevel/dbj_nanobench.h` | dispatch cost measured, not guessed |
| `toplevel/dbj_required_compile_time.h` | the GCC-15-only gate holds |
| `third_party/tau` | the simulation is testable without a window |
| `third_party/dbc_assert` | preconditions worth keeping in the build |

## Lineage

`lineage/tec_drakula` — the C++14 ancestor. MIT, © Pedro Foresti Leão,
used with the author's permission. Git-ignored: reference only, never
built, never linked against.

## Vocabulary

**POC (proof-of-concept)** — code written to establish that an approach
works, then kept as the evidence. Not a library, not a product.

**Integration POC** — the same, where the thing being established is
that separately-proved parts compose.

**raylib** — a small C99 game library. <https://www.raylib.com/>
