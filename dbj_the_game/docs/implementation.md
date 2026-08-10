---
version: 0.3
chamber: implementation
spec: dbj_chamber.md
siblings: [design.md, implementation_milestone_one.md, milestones.md]
actors:
  DBJ: { role: [supervisor],       kind: human, writes: rulings }
  ASH: { role: [author, reviewer], kind: agent, writes: objections and answers }
  ZED: { role: [author, reviewer], kind: agent, writes: objections and answers }
signal:
  ASH: false
  ZED: true
protocol:
  - One collapsed <details> block per actor, id = actor name.
  - One line per item, opening with [settled] | [fix] | [open].
  - Nobody edits another actor's block.
  - DBJ rules on [open] items when every signal is true, then resets them.
---

# dbj_the_game — implementation chamber

The *how*. [design.md](design.md) holds the shape and the reasoning; this
file holds what a person needs to actually build the thing — toolchain,
vendored libraries, link lines, derived constants, and which header attaches
where.

Split this way on purpose: everything here can change without the design
changing. A different raylib version, a different link order, a bigger map
file — none of it touches the tag-and-switch argument next door.

## Discussion

Actors, rules and signal state are in this file's front matter; the protocol
itself is specified in [dbj_chamber.md](dbj_chamber.md). Same room as
[design.md](design.md), same protocol — only the subject differs.

---

<details id="DBJ_to_ASH_and_ZED" markdown="1">
<summary>All in this group consider <b>[settled]</b> by <b>DBJ</b></summary>

Rulings already made that land in this file. Carried from
[design.md](design.md#discussion), not restated by anyone else.

**raylib** NOT is not to follow the `libcurl` but genuinely vendor both halves through $(OS) in Make

**raylib** - this is windows/linux repo on both we will do static linking. when and which we will decide in a Makefile. Install raylib win and linux libs under `\third_party` in separate folder of course

**nanobench row** - I could not care less. So ASH save and commit

</details>

---

<details id="ZED_notes" markdown="1">
<summary><b>ZED</b> — implementation notes</summary>

**SIGNAL** — DBJ should intervene — FALSE.

**[settled] build dir** — `DBJ_BUILDS` with `../builds` fallback, as every
other Makefile here does; the game is not special.

**[settled] compiler pin** — `gcc` on Windows, `gcc-15` on Linux, per
`tribute_to_tony/Makefile`: distro `gcc` still defaults to older.

**[settled] link order** — archive by path first, platform libraries after,
both recorded in *Build*.

**[settled] arena constants** — enemy cap and `enemies[16]` live together in
`world.c`, so neither can be edited alone.

**[open] raylib not vendored** — the hard blocker; I can fetch the 5.5
prebuilts, verify SHA256 and write the manifest, but it commits binaries so
DBJ says go.

**[open] Linux half is unverifiable here** — this machine is Windows-only, so
`linux64/libraylib.a` ships committed but untested; the manifest must say so
rather than imply both were checked.

**[settled] raylib vendored** — 5.5 both halves, static, verified linking
against GCC 15.2; only `libraylib.a` taken, never the `libraylibdll.a` beside
it.

**[settled] Makefile** — written, `$(OS)` split, archive named by path.

**[settled] axis-separated collision** — resolving a 2D overlap in one pass
has to guess which axis the mover entered by, and guesses wrong at every tile
seam; `collide_map` moves and resolves X, then Y, which removes the guess.

**[settled] dt clamp** — `main.c` caps frame time at 1/30s, or a stall
tunnels movers through platforms, collision here being discrete.

**[settled] hazards do not block** — spikes and fire are reported through an
out-parameter and never resolved, so they damage without stopping a mover.

**[settled] map authoring constraint** — a platform one row above the floor
is a wall, not a step: the player is 43px and cells are 40px, so any
single-cell rise blocks a runner. Ground-floor rows must stay clear.

**[open] tau tests** — the simulation is driveable headless (proven with
throwaway probes) but no `tau` suite is committed yet.

</details>

---

<details id="ASH_notes" markdown="1">
<summary><b>ASH</b> — implementation notes</summary>

**[settled] raylib link** — name the archive by path, never `-lraylib`: an
import library beside the static one links clean and dies at startup.

**[settled] link order** — static raylib needs the platform libraries *after*
it, or `ld` reports undefined references from code that is plainly present.

**[settled] provenance** — `third_party/raylib/` owes a `readme.md` and a
`BUILD-MANIFEST.txt`, as `third_party/libcurl/` carries; two archives, two
entries.

**[settled] vendoring both halves** — with no `pkg-config` path, GCC 15 stays
the only machine-local dependency, and that is already enforced at compile
time by `dbj_required_compile_time.h`.

**[settled] arena sizes** — the original numbers were asserted, not derived,
which trades a leak for a silent spawn failure.

**[settled] projectiles** — derived from `Archer.cpp`'s one-second attack
timer and view-bounded flight: ~20 in flight, so 64 is ~3x headroom.

**[settled] enemy cap** — `maxEnemies` limits enemies *alive at once*, not the
level total, so `enemies[16]` must be derived from it in the same file.

**[settled] loud refusal** — a full arena returns null and asserts in debug;
a fixed arena is only a virtue if overflow is visible.

**[settled] nanobench** — the cross-language comparison was mine and
unfalsifiable across two compilers and two renderers.

**[settled] fonts** — raylib's built-in font removes the two `.ttf` files and
the separate licence question they carry.

**[settled] raylib is installed** — both archives now exist under
`third_party/raylib/` with the manifest beside them; the one hard blocker is
gone.

**[settled] Makefile exists** — the `$(OS)` split specified here is written,
on the `dbjobserve/Makefile` precedent.

</details>

---

## Contents

- [Build](#build)
- [Where the toolkit lands](#where-the-toolkit-lands)
- [Derived constants](#derived-constants)
- [Vocabulary](#vocabulary)

## Build

One `Makefile`, GNU make, GCC 15+. This repo builds on **both Windows and
Linux**, so the Makefile picks the platform; nothing else in the tree knows
which one it is.

| | |
|---|---|
| Compiler | GCC 15.3.0, `-std=c23` — whatever `gcc` PATH resolves to |
| Library | raylib 5.x, **statically linked**, vendored under `third_party/` |
| Warnings | `-Wall -Wextra -Wswitch -Werror` — the exhaustive-switch guarantee depends on these |

**No machine-local absolute path appears in this document.** An earlier
version named the compiler as `G:\mingw64\bin\gcc.exe`; that drive died and
took the toolchain with it, leaving a build instruction that was false and
that nothing would have caught. The Makefile needs a path, a document does
not — it needs a version and a language standard.

Two environment variables carry the machine-local part instead, set once per
developer, never committed:

| Variable | Value here | Why it is not in the Makefile |
|---|---|---|
| `PATH` | must contain the toolchain `bin` (`D:\mingw\bin`) | where GCC is installed differs per machine and per platform |
| `DBJ_BUILDS` | `D:\REPOS\DBJARH\DBJ_CODING_SKILLS\builds` | every folder here builds into one tree; unset falls back to `../builds` |

On Windows both are user environment variables
(`[Environment]::SetEnvironmentVariable(..., "User")`), which only new shells
see — an already-running shell keeps the environment it started with.

**Bumping GCC means re-running the suite.** The pinned version is the one the
build is known good against, not a floor. A new major version changes
diagnostics, and this build treats every warning as an error, so a bump can
fail the build outright — and can change behaviour the suite exists to pin
down. Run [`tests/dbj_the_game_test.c`](../tests/dbj_the_game_test.c) via
`make test` and record the new version here in the same commit.

**raylib is vendored, not installed.** It lives in the repo alongside the
other third-party code, one folder per platform, so a clone builds without a
machine-local prerequisite:

```
third_party/raylib/
  readme.md            origin, version, license
  BUILD-MANIFEST.txt   SHA256 of each archive
  include/raylib.h
  win64/libraylib.a
  linux64/libraylib.a
```

`readme.md` and `BUILD-MANIFEST.txt` are not optional — `third_party/libcurl/`
carries both, and a vendored binary without an origin URL, a version and a
checksum is a thing nobody can audit or re-fetch. This one commits two
archives, so it owes two entries.

Static only. There is no runtime DLL or `.so` to ship, nothing to put on
`PATH`, and no version skew between the header and whatever the system had
lying around — which is exactly the failure the ancestor's `make.cmd`
prepending `G:\SFML` was working around.

Static raylib does not link on its own; it needs the platform's graphics and
timing libraries after it, and the order matters to `ld`:

| | |
|---|---|
| Windows | `third_party/raylib/win64/libraylib.a -lopengl32 -lgdi32 -lwinmm` |
| Linux | `third_party/raylib/linux64/libraylib.a -lGL -lm -lpthread -ldl -lrt -lX11` |

**Name the archive by path, never `-lraylib`.** If an import library ever
lands beside the static one, `ld` silently prefers it: the build succeeds and
the binary dies at startup on a missing DLL. `dbjobserve/Makefile:19` carries
that comment for `libcurl` for exactly this reason.

Which toolchain and which of the two library folders is a decision the
Makefile makes at build time. **Nothing is installed yet** — this is the one
hard blocker before any of the above compiles.

## Where the toolkit lands

The [readme](../readme.md) commits to nine artefacts. This is where each one
attaches, and what it would take for the attachment to count as *proved*
rather than merely *used*.

| Artefact | Attaches at | Proved when |
|---|---|---|
| `dbj_defer.h` | `FILE*` in `map.c`; texture handles in `draw.c` | the only two places the simulation owns a resource — if defer is awkward here, it is awkward everywhere |
| `dbj_result.h` | `map_load`, asset loading | a missing map file reports a reason and unwinds, instead of `exit(1)` |
| `dbj_simple_log.h` | startup, asset resolution, spawn refusals | a full arena is visible in the log, not silent |
| `dbj_clintro.h` | banner in `main` | trivial, one call |
| `dbj_macros.h` | `DBJ_LOOP_AS` over arenas and the map grid | reads better than the plain `for` it replaces, at 4 nested loops in `physics.c` |
| `dbj_nanobench.h` | `entity_step` over a synthetic arena | dispatch cost measured same-compiler, same-flags — **not** against the C++ ancestor |
| `dbj_required_compile_time.h` | GCC-15 gate | same as every other folder |
| `tau` | simulation unit tests | a recorded input array replays a game with no window — the reason `input.c` is split out |
| `dbc_assert` | spawn / despawn preconditions | slot index in range, tag matches arena |

Two of these are load-bearing for the design rather than incidental.
`tau` requires the simulation to be reachable without raylib, which is why
input is its own module. `dbc_assert` on spawn is what turns a full arena
from a silent drop into a caught bug.

## Derived constants

The design says arena sizes are derived, not chosen. This is the derivation.
Both ancestor map files are 12 lines x 100 columns at 40 px per cell:

| | `castle.txt` | `cemiterio.txt` |
|---|---|---|
| platforms (`1` / `0`) | 242 | 252 |
| spawn points (`&`) | 65 | 60 |
| spikes / fire in file (`2` / `3`) | 0 | 0 |

**Obstacles 320** = 252 platforms + `Spawner::maxObstacles` (10), with
headroom.

**Enemies 16** = `Spawner::maxEnemies` (15) + 1. This is a *concurrency* cap,
not a level total: `Stage::update` calls `spawnEnemy()` every frame and
`decrementEnemiesCount()` runs on death, so the stage refills forever and
never exceeds 15 alive.

**Projectiles 64** is derived indirectly, since nothing in the ancestor bounds
projectiles in flight directly: `Archer.cpp` fires on `attackTimer >= 1`, one
arrow per archer per second; `Projectile.cpp:42` despawns against the view, so
flight time is under two seconds; enemy kind is `rand() % 2`, so about 8 of
the 15 concurrent enemies are archers. That is ~16 in flight, ~20 with player
knives — 64 is roughly **3x a derived worst case**, not a round number.
Stated so anyone shrinking it knows what they are cutting into.

**Spawn points 128** = 65, the larger ancestor map's `&` count, doubled. It
had been `OBSTACLES_MAX`, which is a different quantity: a map dense in `&`
and thin in `1` would size the spawn array off a number that says nothing
about it.

**Where the cap lives.** The concurrency constant belongs in `world.c` beside
`world_respawn`, with the arena size derived from it (`enemies[16]` = cap 15
+ 1) and a comment tying the two together. Editing one without the other is
the bug worth preventing.

**Refusal must be loud.** A full arena makes `world_spawn` return null, and
debug builds assert. A silently dropped enemy is exactly the bug class the
design claims to remove — a fixed arena trades a leak for a silent spawn
failure unless refusal is visible.

## Vocabulary

**Import library** — on Windows, a small `.a`/`.lib` that resolves symbols
against a DLL loaded at run time, rather than embedding the code. Linking one
by accident produces a binary that builds but will not start.

**Link order** — `ld` resolves each archive against symbols still undefined at
the point it is read, so a library must appear *after* whatever needs it.
Wrong order gives undefined references from code that is plainly present.

**`pkg-config --static`** — the usual way to get a distro library's full
transitive link line. Not used here: both raylib halves are vendored, so GCC
15 stays the only machine-local dependency.

**Vendoring** — committing a third-party binary or source into the repo
instead of depending on it being installed. Costs git history, buys a clone
that builds.

---

(c) 2026 by dbj@dbj.org | MIT license
