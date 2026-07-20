# Contributing

> **Caveat Emptor**: We are enjoying the metapresence of [DBJ Taxonomies](https://method.dbj.org/taxonomy_core.html). Thus we know where are we in the information space with these endeavor. Top category: **Implementation**. Capability: **Development**. In other word: We know what is this all about. And we can explain it to the reader.

## What this repo is

Small, self-contained C proof-of-concepts, not a build/library project.
Each folder is a standalone example — see [README.md](README.md) for
the current list and [CLAUDE.md](CLAUDE.md) for the design principles
every example follows.

## Compiler

GCC 15 or better is the *only* supported compiler — Clang, MSVC, and
everything else are explicitly not supported (several headers rely on
GCC extensions, e.g. nested functions via
[`toplevel/dbj_defer.h`](toplevel/dbj_defer.h), that Clang does not
implement). This is enforced at compile time, not just documented:
[`toplevel/dbj_required_compile_time.h`](toplevel/dbj_required_compile_time.h)
`#error`s out immediately on Clang or on GCC older than 15.

On Windows, MinGW-w64 GCC works well — get it from
[winlibs.com](https://winlibs.com) (download the latest GCC release,
extract anywhere, e.g. `G:\mingw64`).

## Building

See [build.md](build.md) — each folder has its own `Makefile`;
`build.cmd` / `build.sh` at the repo root are thin wrappers that drive
them.

## IntelliSense warning

VS Code's IntelliSense may show false errors (e.g. `uint8_t is not a
type name`, `constexpr is undefined`, or squiggles under
`[[gnu::cleanup]]`/nested functions in `dbj_defer.h`) even though the
code compiles fine under GCC. This happens because IntelliSense hasn't
indexed the GCC headers, and it can't model some GNU-only extensions
at all — these are known false positives, not real errors.

## Before changing code

Read [CLAUDE.md](CLAUDE.md) first — it defines the repo's core
principles (only factory methods, no exceptions, no OOP, tagged
unions with exhaustive `switch`, etc.) and the working conventions
that apply across every folder. Design before code: work out the
approach and check it against the relevant folder's requirements/design
docs before editing — see CLAUDE.md's "Primary objective" section.
