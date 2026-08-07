---
version: 0.1
---

# dbj_the_game

A C23 + raylib rewrite of a C++14/SFML side-scroller.
Design: [docs/design.md](docs/design.md).

**Status: design only. Nothing is built yet** — raylib is not installed
on this machine. See the design's *Build* section.

## Why this is not just a game

The other folders in [this repo](../README.md) are proofs-of-concept in
the strict sense: one idea, one file, self-contained. This one is a POC
too, but of a different thing.

What the small examples cannot show is whether
[toplevel/](../toplevel/) and [third_party/](../third_party/) survive
contact with a real program — several headers in use at once, thousands
of lines, an external library, a frame budget. A header that is pleasant
in a fifty-line demo and unusable at size has not actually been proved.

So the game is the load, and the toolkit is the subject.

## What it is meant to exercise

Intent, not yet fact — this table is a design commitment to be checked
off as the code lands.

| Artefact | Exercised by |
|---|---|
| `toplevel/dbj_defer.h` | file handle in `map.c`, raylib texture handles in `draw.c` — the only places the simulation owns a resource |
| `toplevel/dbj_result.h` | `map_load` and asset loading — the failure paths, where a game usually just exits |
| `toplevel/dbj_simple_log.h` | startup, asset resolution, spawn refusals |
| `toplevel/dbj_clintro.h` | the banner in `main` |
| `toplevel/dbj_macros.h` | `DBJ_LOOP_AS` over the arenas and the map grid |
| `toplevel/dbj_nanobench.h` | dispatch cost of `entity_step` over a synthetic arena — same compiler, same flags |
| `toplevel/dbj_required_compile_time.h` | GCC-15-only gate, same as everything else here |
| `third_party/tau` | unit tests over the simulation — which is plain C on plain data, so it tests without a window |
| `third_party/dbc_assert` | preconditions on spawn and despawn |

The repo's [core principles](../CLAUDE.md#core-coding-principles) apply
unchanged. Two of them are the whole point of the exercise: principle 9
(`static` + size on every array parameter) and the no-`default` `switch`
that makes an unhandled entity kind a compile error.

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
