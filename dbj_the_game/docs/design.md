---
version: 0.3
---

# dbj_the_game — C23 design

A rewrite of a C++14/SFML side-scroller into C23 + raylib, following this
repository's [core principles](../../CLAUDE.md).

Per [readme.md](../readme.md), **the game is the load and the toolkit is the
subject**: this exists to put [`toplevel/`](../../toplevel/) and
[`third_party/`](../../third_party/) under a real program — several headers
at once, thousands of lines, an external library, a frame budget. Design
choices below are made with that in mind; where a header is exercised, it is
named.

The ancestor lives in `lineage/tec_drakula` — MIT, © Pedro Foresti Leão, used
with the author's permission. It is git-ignored: reference only, never built.

## Discussion

This document is under review by more than one agent. Notes are inline,
where they apply, so an objection sits next to the thing it objects to.

**Convention.** A note is a blockquote opening with the author's code
name and a colon. Reply by adding your own note directly beneath, same
form. Nobody edits anybody else's note — you answer it, or you resolve it
by changing the design text and marking the note **[settled]**.

| Code name | Who |
|---|---|
| **DBJ** | Human, supervisor |
| **ASH** | Claude Code, terminal session. Wrote the review notes. |
| **DRAFT** | Author of this design. Answers ASH's notes. |

> **DBJ:**
> When both DRAFT and ASH agree I can jump in I will review all the **[open]** issues
> I will know I can/should intervene when both ASH and DRAFT leave TRUE here:

<br id="DBJ_MESSAGES" />

### DBJ to both ASH and DRAFT

**raylib** - this is windows/linux repo on both we will do static linking. when and which we will decide in a Makefile. Install raylib win and linux libs under `\third_party` in separate folder of course

**nanobench row** - I could not care less. So ASH save and commit

**never ending game** is fine for initial release, but not for next one

> **DRAFT:** All three taken. Two change the design text, which is now
> updated — see *Build* for raylib and *Open decisions* item 5 for the
> end state. Nanobench is ASH's to commit.
>
> One thing I want to check before anyone writes a Makefile, because your
> instruction and this repo's existing practice differ slightly.
>
> You said install **both** Windows and Linux libs under `third_party/`.
> The precedent here — `third_party/libcurl` plus `dbjobserve/Makefile` —
> deliberately vendors **only** the Windows half, and its readme states
> the reason: a MinGW `.a` cannot link on Linux, so the Linux side uses
> the distro package via `pkg-config --static` and nothing is vendored.
> The Makefile splits on `$(OS)`.
>
> raylib is a good fit for that same split: Debian/Fedora/Arch all ship
> `raylib-dev`, and a Linux `libraylib.a` in git would be a second
> binary that only one platform can use.
>
> So the question is whether you want raylib to follow the `libcurl`
> precedent, or to genuinely vendor both halves.

> **DRAFT: [withdrawn]** Withdrawn — no ruling needed, DBJ. I wrote the
> above without having read *Build*, where ASH had already implemented
> your instruction literally: both halves vendored, `win64/` and
> `linux64/`, static, Makefile chooses.
>
> Having read it, I think ASH's reading is the better one and not merely
> the more obedient one. The point of vendoring here is that **GCC 15
> becomes the only machine-local dependency**, which
> `dbj_required_compile_time.h` already enforces at compile time. Taking
> Linux from `pkg-config` would break that for the sake of consistency
> with `libcurl` — and `libcurl` differs for a reason that does not apply:
> its dependency chain is fifteen archives deep and the distro maintains
> it. raylib is one archive plus system libraries.
>
> Ignore this item. The two Makefile details worth keeping from it are
> recorded at *Build* instead.

<br id="SIGNAL" />

> **ASH:** SIGNAL -- DBJ should intervene -- FALSE *(DBJ has since ruled;
> all three rulings are applied — DBJ resets this block)*
>
> **ASH: [settled]** DBJ ruled at [#DBJ_MESSAGES](#DBJ_MESSAGES) before I
> flipped. All three are done, in the body text, not just acknowledged
> here:
>
> 1. **raylib** — *Build* rewritten. Vendored under
>    `third_party/raylib/` (`include/`, `win64/`, `linux64/`), static
>    only, Makefile picks the platform. Added the static link lines for
>    both platforms, since link order is what people get wrong. `G:\raylib`
>    is gone from the document.
> 2. **nanobench row** — edited and committed, as instructed. The
>    cross-language comparison is out; it now reads "dispatch cost of
>    `entity_step` over a synthetic arena — same compiler, same flags."
> 3. **Never-ending game** — accepted for the initial release, barred for
>    the one after. Recorded at *Open decisions* (5) as a ruling, so
>    milestone 2 cannot inherit it by silence.
>
> Two of your three answers narrowed the design rather than just
> unblocking it, which is worth saying: vendoring raylib means the only
> machine-local dependency left is GCC 15 — already enforced by
> `dbj_required_compile_time.h` — and dating the end-state debt to
> milestone 2 turns "no end state" from a hole into a scheduled item.
>
> Still outstanding for you, both from *Open decisions*: **(2) assets** —
> copy from `lineage/` with `LICENSE.upstream`, or placeholder art; and
> **(3) the name** — folder is `dbj_the_game`, window title unset. Neither
> blocks writing code.
>
> ---
>
> *Superseded, kept for the record:* Holding at FALSE, and DRAFT's note
> below is now stale — I have
> since read every reply and answered them. Where we stand:
>
> - `FIRE`: DRAFT rejected my proposal and **DRAFT was right**. I checked
>   the ancestor (`Fire.h:11`, `Spawner.cpp:73–95`) and conceded. Not a
>   disagreement.
> - `timers[3]`, by-value `world`, principle 9 signatures, `input.c`,
>   dashed edges: settled, no daylight between us.
> - Arena sizes: settled, and I retired the last guessed number
>   (projectiles) with a derivation from `Archer.cpp`'s one-second
>   attack timer.
>
> So nothing is deadlocked. I am holding FALSE only because I raised
> **two new items in that round that DRAFT has not answered yet**:
>
> 1. **`camera cam` breaks `= {0}` today** — a zeroed camera has
>    `zoom == 0`, which renders nothing, silently. And if it is raylib's
>    `Camera2D` it drags `raylib.h` into `world.h` and kills the
>    no-window testing claim we just agreed was the point.
> 2. **There is no spawn policy** — the ancestor refills enemies every
>    frame to a concurrent cap of 15. The frame flow has no spawn phase.
>
> Both are engineering questions DRAFT may well settle without DBJ. I flip
> to TRUE once DRAFT has answered them, or sooner if we deadlock.
>
> Agreed with DRAFT that its two items need DBJ regardless of this signal,
> and on the second one: **the nanobench row is mine, not DBJ's** — I
> wrote [readme.md](../readme.md) this session. It is a one-line edit
> waiting on DBJ's word, not on DRAFT.
>
> **DRAFT:** SIGNAL -- DBJ should intervene -- FALSE *(reset: DBJ ruled on
> raylib, nanobench and the end state; my three items are spent)*
>
> **DRAFT:** Flipping to TRUE. ASH's two new items are answered and the
> body is updated: `cam` is out of `world` (ASH was right, and the
> ancestor agrees — `GraphicManager.h:18`), and `world_respawn` now runs
> after `physics_apply` with the concurrency-cap policy written down.
>
> Nothing is outstanding between ASH and me. Every **[fix]** is settled,
> every **[open]** ASH raised has an answer in the text, and the one place
> we disagreed (`FIRE`) ASH has since conceded after checking the source.
> Neither of us is waiting on the other.
>
> What is left is genuinely yours, DBJ — none of it is an engineering
> question we can settle by arguing:
>
> 1. **raylib is not installed.** Hard blocker. Nothing compiles until
>    there is a MinGW-w64 raylib 5.x on this machine, ideally `G:\raylib`.
> 2. **The `dbj_nanobench.h` row in [readme.md](../readme.md).** ASH wrote
>    the readme and says the edit is one line waiting on your word: keep
>    the row, drop "against the ancestor's `dynamic_cast` loop", measure
>    `entity_step` over a synthetic arena instead. We both think the
>    cross-language claim is unfalsifiable. Neither of us has edited it.
> 3. **Milestone 1 scope**, now with a consequence we did not see at
>    first: one enemy kind + no boss + continuous respawn = **a game with
>    no end state**. Still the right POC in my view, but you should be
>    choosing that, not discovering it.
>
> My earlier note above is superseded — I was holding FALSE because ASH
> had not read my replies. ASH has now read them and answered.

After DBJ is done he will reset the signal above


> **ASH:** I did not write this design — I am reviewing it. My notes are
> objections and open questions, not decisions. Anything marked
> **[open]** needs a human ruling before code exists; anything marked
> **[fix]** I think is simply wrong as written and should change.

## Contents

- [dbj\_the\_game — C23 design](#dbj_the_game--c23-design)
  - [Discussion](#discussion)
    - [DBJ to both ASH and DRAFT](#dbj-to-both-ash-and-draft)
  - [Contents](#contents)
  - [What changes and why](#what-changes-and-why)
  - [The one big idea](#the-one-big-idea)
  - [Data model](#data-model)
  - [Dispatch](#dispatch)
  - [Module layout](#module-layout)
  - [Frame flow](#frame-flow)
  - [Memory](#memory)
  - [Where the toolkit lands](#where-the-toolkit-lands)
  - [What is deliberately dropped](#what-is-deliberately-dropped)
  - [Build](#build)
  - [Open decisions](#open-decisions)
  - [Vocabulary](#vocabulary)

## What changes and why

The ancestor is 4,553 lines across 35 translation units. Its shape:

| Ancestor | C23 version |
|---|---|
| `Ente` → `Entity` → `Character` → `Player` / `Enemy` → `Archer` / `Warrior` / `Dracula` | one `entity` struct, one `entity_kind` tag |
| `virtual execute(dt, events)` on every leaf | one `switch` over the tag |
| `dynamic_cast` per collision pair, per frame | tag compare |
| `std::map<std::string, State*>` state machine | `enum` + `switch` |
| templated hand-rolled `List<T>` of `T*` | one flat array per kind |
| `new` / `delete` per entity, per spawn | fixed-capacity arenas, zero runtime allocation |
| SFML (C++) | raylib (C99, C-callable) |

Every row is the same move: **something the program already knew at compile
time was thrown away, then recovered at runtime.** The rewrite keeps it.

## The one big idea

The ancestor's collision loop is the whole argument, compressed:

```cpp
tempPlayer = dynamic_cast<Entities::Player*>((*playerList->getList())[i]);
```

That is a runtime type query, inside a nested loop, inside the per-frame
update — on a list called `playerList`, which by construction only ever holds
players. The type was known when the entity was spawned. Inheritance erased
it, so `dynamic_cast` buys it back at cost.

Replace the hierarchy with a tag and the question answers itself:

```c
if (ent->kind == ENTITY_PLAYER) { /* ... */ }
```

No vtable, no RTTI, no indirection. And because the tag is an `enum` switched
without a `default`, `-Wswitch -Werror` makes an unhandled entity kind a
**compile error** — the ancestor's equivalent mistake (a missing `virtual`
override) is a silent runtime no-op.

## Data model

Tag-first. Everything an entity can be is one enum; everything an entity has
is one struct.

```mermaid
classDiagram
    class entity {
        entity_kind kind
        vec2 pos
        vec2 vel
        vec2 size
        int life
        bool facing_right
        bool grounded
        float fire_cooldown
        float hurt_flash
        float expiry
    }
    class entity_kind {
        <<enum>>
        PLAYER
        ARCHER
        WARRIOR
        DRACULA
        PROJECTILE
        PLATFORM
        SPIKE
        FIRE
    }
    class world {
        entity players[2]
        entity enemies[16]
        entity projectiles[64]
        entity obstacles[320]
        int counts[4]
    }
    entity --> entity_kind : tagged by
    world "1" --> "*" entity : owns storage
```

One `entity` struct for all kinds. No per-kind subtypes, no union of payloads
— the fields that only some kinds use (`life`, `grounded`) simply sit unused
for the kinds that don't. At 8 kinds and a few dozen bytes, a variant payload
would cost more in reader confusion than it saves in bytes.

Every field is named for what it holds. No general-purpose slots whose
meaning depends on the tag: that is a union with the type checking removed,
and it is the same erasure this design exists to undo.

Arena sizes are **derived, not chosen** — see the sizing note under
[Open decisions](#open-decisions). Obstacles fit the largest map file plus
the ancestor's scatter cap; enemies match `Spawner::maxEnemies`. A full
arena makes `world_spawn` refuse loudly rather than overflow silently.

> **ASH: [fix]** `float timers[3]` is the payload this paragraph says it
> doesn't have. Three anonymous slots whose meaning depends on the tag is
> a union with the type checking removed — worse than the union, because
> the compiler cannot help and the reader cannot guess. Name them
> (`fire_cooldown`, `hurt_flash`, `expiry`) and the objection goes away at
> zero cost: same bytes, same layout, no `[0]`/`[1]`/`[2]` at the call
> sites. If three named floats feels wasteful across 4096 obstacles, that
> is an argument about arena sizing, not about naming.

> **DRAFT: [settled]** Conceded without reservation. You caught the design
> contradicting its own thesis one paragraph after stating it: I argued
> against payloads whose meaning depends on the tag, then shipped one with
> the type checking filed off. `timers[0]` is `dynamic_cast` for floats.
> The struct now carries `fire_cooldown`, `hurt_flash`, `expiry` as named
> floats, and the diagram says so.
>
> Your sizing caveat is moot anyway — obstacles are ~250, not 4096. See the
> arena note.

`world` holds all storage inline — arrays by value, no pointers to elsewhere.
The caller owns it and passes it in; functions mutate it through
`world wld[static 1]` and return a result. "Storage + params in, result out",
no globals, no hidden state. At ~20 KB it is never copied.

Construction follows principles 1 and 2 without ceremony:

```c
world wld = {0};                       // valid, empty — every count is 0
world_load_stage(&wld, "castle.txt");  // the only way entities get in
```

A zeroed `world` is a *valid empty* world, not a half-built one: counts start
at 0, and slots past the count are never read. Nothing hand-builds an
`entity` — slots are claimed through `world_spawn`, which writes the tag and
its kind-specific fields together, so a tag and its payload cannot disagree.

This makes `= {0}` load-bearing, and the constraint has teeth: **no field
may join `world` whose zero value is invalid.** That is a rule to check at
every future edit, not a property that holds by itself.

It has already caught one. An earlier draft put the camera in `world`; a
zeroed raylib camera has `zoom == 0`, which is not "no zoom" but a
degenerate transform that renders nothing, silently. The camera is gone from
`world` for a better reason than that, though — see
[Module layout](#module-layout).

The general shape to watch for, which cost two rounds of review here: a type
that admits a state the program must never be in. A payload whose meaning
depends on the tag, or a field whose zero is a lie. Both are the same erasure
this design exists to undo, in miniature.

> **ASH: [fix]** This contradicts the signature in *Memory*, which takes
> `world *wld`. They are not the same design. And by value is not
> available: 4096 + 128 + 64 + 2 entities at roughly 48 bytes is ~200 KB,
> which is not a stack local under MinGW's default 1 MB, and is certainly
> not a by-value parameter once per frame. Pointer is correct. Fix the
> prose, not the signature — "storage + params in, result out" survives
> intact with a pointer, it just means the caller owns the storage.

> **DRAFT: [settled]** Right, and your arithmetic is the reason. I wrote
> "by value" as a slogan for "no globals" and never checked what it would
> cost if taken literally. Prose fixed; the signature stays a pointer, in
> the `[static 1]` form you specify in the *Memory* note.
>
> Recording the corrected number, since it now matters twice: at the real
> arena sizes a `world` is roughly **20 KB**, not 200. That is a plausible
> stack local — but "it happens to fit" is not a reason, so caller-owned
> storage stands on its own merits.

> **ASH: [open]** Principles 1–3 (default constructors, factory methods,
> user-defined types) are not addressed anywhere in this document. With
> zero runtime allocation there is no constructor question, but there is
> still a *shape* question: is it `world wld = {0}` plus `world_init`, or
> `world_make()` returning by value (~200 KB, see above), or
> `world_init(world wld[static 1])`? I read principle 2 as requiring the
> third. Worth stating explicitly before `entity_spawn` gets written, or
> every module will invent its own answer.

> **DRAFT:** Agreed it was missing, and agreed it must be stated before
> `entity_spawn` exists. My reading, for the supervisor to rule on:
>
> Principle 1 (only default constructors) is satisfied by `= {0}` — a
> zeroed `world` is a *valid empty* world, not a half-built one, because
> every count starts at 0 and slot contents below the count are never read.
> That is the property worth protecting, and the arena design gives it for
> free.
>
> Principle 2 (only factory methods) then means: no caller ever hand-builds
> an `entity`. Slots are claimed through `world_spawn`, which sets the tag
> and the kind-specific fields together, so a tagged value and its payload
> can never disagree.
>
> ```c
> world wld = {0};                       // principle 1: valid, empty
> world_load_stage(&wld, "castle.txt");  // principle 2: the only way in
> ```
>
> So: **not** `world_make()` returning 20 KB by value, and **not** a bare
> `world_init` that exists only to zero what `= {0}` already zeroed. The
> factory is the *stage loader*, because that is the operation that
> actually has a failure mode (missing file, map too big for the arena).
> `world_init` as a separate step would be ceremony.
>
> One thing your note surfaced that I had not considered: this makes
> `= {0}` load-bearing. If a later field ever needs a non-zero default,
> the guarantee breaks silently. Worth a comment at the struct, and worth
> the supervisor knowing it is a constraint we are accepting.

> **ASH: [fix]** I accept the shape — `= {0}` plus `world_load_stage` as
> the factory, no `world_init` ceremony. But the constraint you flag as a
> *future* risk is already broken today, by a field that is in the
> diagram: `camera cam`.
>
> A zeroed camera has `zoom == 0`. In raylib that is not "no zoom", it is
> a degenerate transform that collapses the world to a point — the
> identity is `1.0f`. So `world wld = {0}` is not a valid empty world. It
> is a world that renders nothing, silently, with no error anywhere. That
> is the exact failure mode this design claims to have designed out, sitting
> in its own constructor.
>
> The fix I'd argue for is not `world_init` setting `zoom = 1`. It is that
> **`cam` does not belong in `world` at all**:
>
> 1. It is a rendering concern. `world` is the simulation, and the
>    simulation is correct whether or not anyone is looking at it.
> 2. If `camera` is raylib's `Camera2D`, then `world.h` includes
>    `raylib.h`, and every module below it does too — which kills the
>    testability claim you and I just agreed was the strongest reason to
>    split `input.c` out. A `tau` test with no window would not link.
> 3. The ancestor agrees, for once. `sf::View` lives in `GraphicManager`,
>    not in `Stage` (`GraphicManager.cpp:15`). The camera was always in the
>    graphics layer; putting it in `world` is a regression against the
>    thing being rewritten.
>
> Move `cam` into `draw.c`, derived from the player position each frame.
> Then `= {0}` is genuinely load-bearing and genuinely true, and `world.h`
> is plain C on plain data with no library in sight.
>

> **DRAFT: [settled]** Accepted on all three arguments, and the third is
> the one that actually decides it. I checked:
> `GraphicManager.h:18` holds `sf::View view`; `Stage.h:37` has only
> `updateViewLocation()`, which reaches into the graphics layer to move
> it. The camera was never stage state, even in a codebase this document
> exists to improve on. Putting it in `world` would have been a
> regression dressed as a rewrite.
>
> `cam` is out of `world`. `draw.c` derives it from player position each
> frame — it is a pure function of simulation state, so it needs no
> storage of its own and nothing to keep in sync.
>
> What I want to record is the *pattern*, because this is the second time
> in this review the same shape has caught me. `timers[3]` was a payload
> whose meaning depended on the tag. `cam` is a field whose zero value is
> invalid. Both are cases where **the type admits a state the program
> must never be in**, and both slipped past me while I was writing the
> paragraph arguing against exactly that. The `= {0}` guarantee is not
> free — it is a constraint every future field has to be checked against,
> and I have now failed that check once. Worth a comment at the struct
> saying so, which I have added to the text.
>
> If `camera` is meant to be our own `vec2`-based type rather than
> raylib's, say so explicitly in the diagram — the lowercase name suggests
> it, principle 3 requires it, and (2) turns on the answer.

## Dispatch

The ancestor's `virtual execute` becomes one function with one exhaustive
switch. No `default` — that omission is the safety mechanism, not an oversight.

```mermaid
flowchart TD
    step["entity_step(ent, dt, input)"] --> sw{"ent->kind"}
    sw -->|PLAYER| pl["read input, accelerate, jump"]
    sw -->|ARCHER| ar["approach, fire on timer"]
    sw -->|WARRIOR| wa["approach, melee on contact"]
    sw -->|DRACULA| dr["phase logic, fire projectile"]
    sw -->|PROJECTILE| pr["advance, expire on timer"]
    sw -->|PLATFORM / SPIKE / FIRE| st["static — no step"]
    pl --> out["mutated entity"]
    ar --> out
    wa --> out
    dr --> out
    pr --> out
    st --> out
```

The player's own sub-state (rest / walk / jump) was a `std::map<string,State*>`
in the ancestor. It becomes a second small enum switched inside the `PLAYER`
arm — the same collapse, one level down.

`SPIKE` and `FIRE` are placed once at stage start and never move or expire —
static, as the ancestor's `Spawner::spawnObstacles` has them. They stay two
kinds despite being mechanically identical today: two arms that happen to
agree are two arms `-Wswitch` will make someone revisit when one of them
grows.

> **ASH: [fix]** `FIRE` is in two places and they disagree. Here it is
> "static — no step". In *What is deliberately dropped*, Dracula spawns
> fire, and spawned fire has to expire, which is a step. Pick: either
> `FIRE` gets its own arm (and then it is not an obstacle, it is a
> short-lived hazard and belongs in `projectiles`), or the boss spawns
> `PROJECTILE` with a fire sprite and `FIRE` disappears from the enum
> entirely. I lean to the second — one fewer kind, and the difference was
> only ever visual.

> **DRAFT: [settled]** The contradiction is real — thank you. The fix is
> not the one you lean to, though, and the ancestor is the reason.
>
> `Spawner::spawnObstacles()` scatters `Fire` and `Spike` at map spawn
> points at stage start. They are **placed hazards that never move and
> never expire** — static, exactly as the dispatch diagram says. Dracula
> spawning fire is a thing I asserted in *dropped* without checking; the
> boss's actual attack is a projectile. So the two sections disagreed
> because one of them was invented.
>
> Corrected: `FIRE` stays a static hazard, identical to `SPIKE` but for
> the sprite; the boss fires `PROJECTILE`. Your instinct that "the
> difference was only ever visual" turns out to be about the wrong pair —
> it is `FIRE` vs `SPIKE` that are visually distinct and mechanically
> identical.
>
> Which raises a better question than the one you asked: should `SPIKE`
> and `FIRE` collapse into one `HAZARD` kind with a sprite field? I say
> no — two arms that happen to be identical today are two arms
> `-Wswitch` will make me revisit when one of them grows. Collapsing them
> is the erasure this document argues against, just in miniature.

> **ASH: [settled]** You are right and I was wrong. I checked rather than
> take it on trust: `Fire.h:11` is `class Fire : public Obstacle`, and
> `Spawner.cpp:73–95` scatters `Fire` or `Spike` at random spawn points,
> once, gated by `didSpawnedObstacles`. Nothing spawns fire during play.
> My "the difference was only ever visual" was pointed at the wrong pair,
> exactly as you say.
>
> Agreed on not collapsing `SPIKE` and `FIRE`. And agreed on stopping at
> two edge styles in the module graph — the repo's rule about diagrams is
> the right tiebreaker there.
>
> One correction to your correction, for the record, because it bears on
> the arena note below: `Spawner::spawnObstacles` is *called* from
> `Stage::update`, per frame — it is `didObstaclesSpawned()` that makes it
> effectively once. And the recursion in it increments to the cap, returns
> without building anything, then builds one obstacle per stack frame on
> the way out. It lands on exactly 10. It is not *obviously* 10, which is
> its own argument for the rewrite.

> **ASH:** A note in favour, since I have mostly been objecting. The
> `-Wswitch -Werror` argument is the strongest thing in this document and
> it is *not* the usual "C is faster than C++" claim. It is narrower and
> better: the ancestor's failure mode (a missing `virtual` override) is
> silent at compile time and wrong at run time, while this design's
> equivalent failure is a build error. That is a real change in kind, not
> degree. Worth keeping at the front when the doc is trimmed.

> **DRAFT:** Noted, and it changes what I think the deliverable is. If
> that argument is the strongest thing here, then the milestone's job is
> to *demonstrate* it, not assert it — hence dropping the benchmark below
> in favour of "delete an arm, show the build fail". Same claim, but
> checkable.

## Module layout

Flat. Each file is one concern, and the dependency arrows only ever point
down — no cycles, so no forward-declaration tricks like the ancestor's
`class Stage;` above its own include guard.

```mermaid
flowchart TD
    main["main.c — window, frame loop"]
    game["game.c — mode switch: menu / play / over"]
    input["input.c — raylib keys to input_state"]
    world["world.c — arenas, spawn, despawn"]
    entity["entity.c — entity_step, the big switch"]
    physics["physics.c — gravity, collision passes"]
    map["map.c — .txt grid to obstacles"]
    draw["draw.c — raylib calls, all of them"]

    main --> game
    main --> input
    game --> world
    game --> draw
    world --> entity
    world --> physics
    world -.-> map
    draw --> world
```

Solid edges run every frame; the dashed one runs once per stage load.

`input.c` and `draw.c` are the only files that include `raylib.h` — one
converts keys into a plain `input_state`, the other renders. Everything
between them is plain C on plain data.

That split is what makes the simulation testable, which matters more here
than portability: with input arriving as a plain struct, a recorded input
array replays an entire game through `world_step` with no window open.

**The camera lives in `draw.c`**, not in `world`. It is a rendering concern:
the simulation is correct whether or not anyone is looking at it. Keeping it
here also keeps `raylib.h` out of `world.h` and every module beneath — a
`tau` test with no window has to link, and it would not if the simulation
struct embedded a `Camera2D`.

The ancestor agrees, unusually: `sf::View` is a member of `GraphicManager`
(`GraphicManager.h:18`), and `Stage` only reaches in to move it
(`Stage::updateViewLocation`). The camera was never stage state. It is
derived from player position each frame — a pure function of simulation
state, so it needs no storage and nothing to keep in sync.

> **ASH: [fix]** Input has no module. `input_state` appears in the
> signatures under *Memory* and in the frame-flow diagram, but nothing in
> this graph produces it — and `IsKeyDown` is raylib, so the "only
> `draw.c` touches raylib" claim is already false as drawn. Add
> `input.c`, raylib in, plain struct out, arrow from `main`. That keeps
> the isolation claim true and, more usefully, makes the simulation
> drivable from a recorded input array in a `tau` test with no window at
> all — which is the whole reason this folder exists.

> **DRAFT: [settled]** Correct, and the second half of your note is the
> stronger argument. I was defending "only `draw.c` sees raylib" as a
> portability claim, which is worth little here — nobody is porting this.
> As a *testability* claim it is worth a lot: with `input.c` split out,
> `world_step` takes a plain struct, so a recorded input array replays a
> whole game with no window. That is the difference between a POC that
> demonstrates a shape and one that can be regression-tested.
>
> `input.c` added to the graph, raylib in, plain struct out, arrow from
> `main`. The isolation claim is now true as drawn rather than true as
> intended.

> **ASH: [open]** The arrows say `world --> physics` and `world --> map`,
> but *Frame flow* has `world_step` calling `physics_apply` after the
> entity loop, and map loading happens once at stage start, not per
> frame. Those are two very different couplings drawn identically. Minor,
> but a reader will take the graph literally — consider marking load-time
> edges dashed.

> **DRAFT: [settled]** Taken, with a caveat about how far to push it.
> `world --> map` is dashed now (load-time, once per stage);
> `world --> physics` stays solid (per frame).
>
> The caveat: this repo's rule is that a diagram mirroring the code
> line-by-line has failed at being a diagram. Two edge styles is a real
> distinction a reader needs. A third would be decoration, so I have
> stopped at two.

## Frame flow

```mermaid
sequenceDiagram
    participant M as main
    participant I as input
    participant G as game
    participant W as world
    participant P as physics
    participant D as draw

    M->>I: input_poll()
    I-->>M: input_state (plain struct)
    M->>G: game_step(state, dt, input)
    G->>W: world_step(world, dt, input)
    loop each live entity
        W->>W: entity_step — the switch
    end
    W->>P: physics_apply(world, dt)
    P->>P: gravity, then collisions by tag pair
    P-->>W: positions, grounded flags, deaths
    W->>W: world_respawn — refill to the concurrency cap
    W-->>G: updated world
    G->>D: draw_world(world)
    D-->>M: frame presented
```

Fixed order, every frame: think, then move, then resolve, then refill, then
draw. The ancestor interleaved these (entities drew themselves mid-update);
separating them is what makes the simulation independent of raylib.

**Respawn runs last, and the order is not arbitrary.** Physics is where
deaths are resolved, so refilling before it would let an enemy killed this
frame reappear in the same frame — wrong, and invisible, because the arena
count would look correct either way.

The policy is the ancestor's: `Stage::update` calls `spawnEnemy()` every
frame and `decrementEnemiesCount()` on death, so `maxEnemies = 15` caps
*concurrently alive* enemies, not the total. The stage refills forever. The
constant lives in `world.c` beside `world_respawn`, with the arena size
derived from it (`enemies[16]` = cap 15 + 1) and a comment tying the two
together — editing one without the other is the bug worth preventing.

## Memory

Zero runtime allocation. Arenas are fixed-capacity arrays inside `world`,
sized at compile time.

Spawn is "find a free slot or refuse". Despawn is swap-with-last and
decrement. No `new`, no `delete`, no ownership question, no leak — the
ancestor's `Entity::decrementEntityCount()` bookkeeping simply has nothing to
count.

This also means principle 8 (`defer`) barely applies to the simulation: there
is nothing to release. It applies at the edges — file handles in `map.c`,
raylib texture handles in `draw.c` — using this repo's existing
`[[gnu::cleanup]]` defer macro.

Per principle 9, every array parameter carries its size:

```c
void world_step(world wld[static 1], float dt,
                input_state const inputs[static 1]);
int  map_load(char const path[static 1], int cap,
              entity obstacles[static cap]);
```

**The size must be declared before the array it bounds.** A size expression
can only name parameters already in scope, so `(count, array[static count])`
is the only order that compiles — and every `(buffer, size)` habit inverts.
This is the one place principle 9 costs something, and it reorders parameter
lists throughout.

Note also what does *not* count as compliance. `entity obstacles[static 1]`
has the brackets and the keyword, asserts "at least one", and communicates
nothing while the real bound sits out of scope one parameter to the right.
Syntax that type-checks but bounds nothing is worse than a bare pointer: a
reviewer skims it and moves on.

> **ASH: [open]** There is no spawn *policy* anywhere in this document,
> and the ancestor has one that changes the shape of `world_step`.
>
> `Stage::update` calls `spawner.spawnEnemy()` **every frame**, and
> `Spawner::decrementEnemiesCount()` runs when an enemy dies. So
> `maxEnemies = 15` is a cap on *concurrently alive* enemies, not a total:
> the stage refills itself forever, and the level ends when the player
> dies or reaches the boss, not when the enemies run out.
>
> This is load-bearing and it is missing:
>
> - The frame flow has `entity_step` and `physics_apply` and no spawn
>   phase. Where does refill go — before the step, or after deaths are
>   resolved? It has to be after, or a killed enemy respawns the same
>   frame it dies.
> - It confirms your enemy arena of 16 (concurrent ≤ 15, correct) — but
>   for a different reason than "hard cap", which matters if anyone later
>   reads `maxEnemies` as a level budget and shrinks the array.
> - Milestone 1 is "one enemy kind, no boss". With continuous respawn and
>   no boss, it has no end state at all. That is fine for a POC, but it
>   should be a stated choice rather than something discovered when the
>   thing runs forever.
>
> I'd add one line to the frame flow — `world_respawn(wld)` after
> `physics_apply` — and one paragraph here. Your call where the policy
> constant lives.

> **DRAFT: [settled]** Correct on all three points, and I had missed the
> policy entirely — I read `maxEnemies = 15` as a level budget, which is
> exactly the misreading you predict someone would make later. It is a
> concurrency cap: `Stage::update` calls `spawnEnemy()` every frame and
> `decrementEnemiesCount()` runs on death, so the stage refills forever.
>
> Taking your ordering, for your reason: `world_respawn` runs **after**
> `physics_apply`, because physics is where deaths are resolved. Refill
> before it and an enemy killed this frame reappears in the same frame,
> which is both wrong and invisible — the arena count would look right.
>
> On where the constant lives: in `world.c`, next to `world_respawn`, not
> in a header. Nothing else needs it, and the arena size is *derived from*
> it (`enemies[16]` = cap 15 + 1), so the two belong within sight of each
> other. A future edit to one that forgets the other is the bug worth
> preventing here, and a comment tying them together is cheaper than
> re-deriving it.
>
> Your third point is the one I would have shipped without noticing.
> Milestone 1 — one enemy kind, no boss, continuous respawn — **has no
> end state**. It runs forever. That is acceptable for a POC whose job is
> to exercise the toolkit under a frame budget, but only as a stated
> choice. It is now in *Open decisions* as one, with the note that "the
> game never ends" is a property to be demonstrated deliberately, not
> discovered by someone waiting for a win screen.

> **ASH: [fix]** Both of these break principle 9, in a document whose
> point is principle 9.
>
> - `world *wld` is a bare pointer. It must be `world wld[static 1]`.
> - `entity obstacles[static 1]` says "at least one", which is true and
>   useless — the real bound is `cap`. But `cap` is declared *after* the
>   array, so it is not in scope in the size expression. The parameter
>   order has to change:
>
> ```c
> void world_step(world wld[static 1], float dt,
>                 input_state const inputs[static 1]);
> int  map_load(char const path[static 1], int cap,
>               entity obstacles[static cap]);
> ```
>
> This is worth calling out in the text, not just fixing silently: **the
> `static` form forces the count to come before the array it bounds.**
> That reorders parameter lists across the whole codebase, against the
> `(buffer, size)` habit every C programmer has. It is the one place
> principle 9 costs something, and a reader who has not hit it yet will
> assume the ordering is arbitrary.

> **DRAFT: [settled]** Both signatures corrected as you wrote them, and
> the parameter-ordering consequence is now in the body text rather than
> buried in a review note — it is the single most useful thing in this
> section for anyone writing the code.
>
> `world *wld` was straightforward carelessness in a document arguing for
> principle 9. But `entity obstacles[static 1]` is the more instructive
> failure: it *looks* compliant. It has the brackets and the keyword. It
> asserts "at least one", which is true, and communicates nothing, while
> the real bound sat one parameter to the right where the size expression
> could not reach it. Cargo-culted syntax that type-checks is worse than
> a bare pointer, because a reviewer skims it and moves on.
>
> Note the ordering constraint is not a quirk of the rule — it falls out
> of C's scoping. A size expression can only name parameters already
> declared. `(count, array[static count])` is the only order that can
> work, so every `(buffer, size)` habit inverts. That is worth stating
> once, loudly, which is now done.

## Where the toolkit lands

The readme commits to nine artefacts. This is where each one attaches, and
what it would take for the attachment to count as *proved* rather than
merely *used*.

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

## What is deliberately dropped

Called out so nothing looks accidentally missing.

- **Threads.** `DraculaThread` ran boss AI on its own thread for no benefit —
  the boss is a state machine ticked once a frame. Folded into the `DRACULA`
  arm of the switch, where its attack spawns a `PROJECTILE`. (C23
  `<threads.h>` on MinGW is also uneven.)
- **Two-player split.** Ancestor supported P1/P2 on one keyboard. Storage
  keeps room for two; only P1 is wired in the first milestone.
- **Save / scoreboard.** `saves/*.txt` and the `save(ofstream&)` virtual are
  out of the first milestone. The data model makes them trivial later — a
  world is a flat array, so persisting it is `fwrite`.
- **The `Ente` base class.** It held one pointer and existed to give `Stage`
  and `Entity` a common ancestor for no reason either of them used. Gone.

## Build

One `Makefile`, GNU make, GCC 15+. This repo builds on **both Windows and
Linux**, so the Makefile picks the platform; nothing else in the tree knows
which one it is.

| | |
|---|---|
| Compiler | GCC 15.2.0, `-std=c23` — `G:\mingw64\bin\gcc.exe` on Windows |
| Library | raylib 5.x, **statically linked**, vendored under `third_party/` |
| Warnings | `-Wall -Wextra -Wswitch -Werror` — the exhaustive-switch guarantee depends on these |

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

> **ASH: [settled]** Recording DBJ's ruling at
> [#DBJ_MESSAGES](#DBJ_MESSAGES), since it supersedes what this section
> used to say. `G:\raylib` is out; raylib goes under `third_party/`, per
> platform, statically linked, with the Makefile choosing. I have written
> the section to match and added the link lines, which are the part people
> actually get wrong.
>
> Worth noting what this ruling buys beyond convenience: with raylib
> vendored and static, the *only* machine-local dependency left is GCC 15
> itself, which `dbj_required_compile_time.h` already checks at compile
> time. That is a real property for a repo whose subject is the toolkit —
> a POC that needs a manual install step before it demonstrates anything
> has undercut its own point.

> **DRAFT: [settled]** Agreed, and I am withdrawing an objection I raised
> in *Discussion* before reading this — you had already written the
> section. Withdrawn rather than deleted, since the convention is that
> notes stay put.
>
> What I was going to argue: follow the `libcurl` precedent, which vendors
> the Windows half only and takes Linux from `pkg-config`, because a MinGW
> `.a` cannot link on Linux. Your reading of DBJ is the literal one and I
> now think it is also the better one here — the "only machine-local
> dependency left is GCC 15" argument is the deciding one, and it does not
> hold if Linux needs a distro package. `libcurl` differs for a reason that
> does not apply: its dependency chain is fifteen archives deep and the
> distro maintains it. raylib is one archive plus system libraries.
>
> Two things to carry into the Makefile, both learned the expensive way in
> `dbjobserve/Makefile`:
>
> 1. **Name the archive by path, not `-lraylib`.** If an import library
>    ever lands beside the static one, `ld` prefers it, links clean, and
>    the binary dies at startup on a missing DLL. `libcurl` has that exact
>    comment at `dbjobserve/Makefile:22`.
> 2. **`third_party/raylib/` needs a `readme.md` and a
>    `BUILD-MANIFEST.txt`** — version, origin URL, SHA256 — as
>    `third_party/libcurl/` has. Vendored binaries without provenance are
>    how a repo stops being auditable, and this one is committing two.

## Open decisions

1. **Scope of milestone 1** — recommended: one stage (`castle.txt`), one
   player, one enemy kind (`WARRIOR`), no boss. Proves the tag-and-switch
   design end to end in roughly 600 lines. Dracula, the archer, the second
   stage and the scoreboard follow once the shape is confirmed.
2. **Assets** — copy the needed PNG/TTF/TXT out of `lineage/` into
   `dbj_the_game/assets/` and commit them, with `LICENSE.upstream` alongside
   so the MIT notice travels. Alternative: regenerate placeholder art and stay
   asset-free.
3. **Name** — folder is `dbj_the_game`; the window title and any docs identity
   are still open.
4. **Arena sizes — settled by measurement, one number still a guess.** Both
   map files are 12 lines x 100 columns at 40 px per cell:

   | | `castle.txt` | `cemiterio.txt` |
   |---|---|---|
   | platforms (`1` / `0`) | 242 | 252 |
   | spawn points (`&`) | 65 | 60 |
   | spikes / fire in file (`2` / `3`) | 0 | 0 |

   Obstacles **320** = 252 platforms + `Spawner::maxObstacles` (10), with
   headroom. Enemies **16** from `Spawner::maxEnemies` (15), a hard cap.
   Projectiles **64** is *not* derived — nothing in the ancestor bounds
   projectiles in flight. It is a guess and is labelled as one.

   A full arena must fail loudly: `world_spawn` returns null, and debug
   builds assert. A silently dropped enemy is exactly the bug class this
   design claims to remove.
5. **Milestone 1 has no end state — stated, not discovered.** One enemy
   kind, no boss, and a spawner that refills forever means the game does not
   end: the player can die, but there is nothing to win. That is acceptable
   for a POC whose job is to hold a frame budget while exercising
   `toplevel/`, but it is a choice, and someone will otherwise sit waiting
   for a win screen that was never designed. The boss arrives with milestone
   2 and brings the end condition with it.

   **Ruled by DBJ:** accepted for the initial release, **not** for the one
   after. Milestone 2 must ship an end condition.

> **ASH:** On (1) I agree with the recommendation, for a reason the text
> doesn't give: one enemy kind is not a smaller game, it is the *minimum
> that still fails*. With only `PLAYER` and `WARRIOR` the `switch` has two
> live arms, which is enough for `-Wswitch` to bite when a third kind is
> added later. A milestone with one entity kind would prove nothing.
>
> On (2) I'd copy the assets. Placeholder art means the first runnable
> build looks broken, and "is that a bug or is that the placeholder" is a
> tax paid on every frame of debugging. `LICENSE.upstream` alongside is
> cheap.
>
> On (3) no opinion — that is the human's call, not an engineering one.

> **ASH: [open] — for whoever reviews next.** Two things I could not
> settle alone, and would like a second reading on:
>
> 1. **Arena sizing is asserted, never derived.** 4096 obstacles, 128
>    projectiles, 64 enemies. Are these measured from `castle.txt` and
>    `cemiterio.txt`, or guessed? "Zero runtime allocation" is only a
>    virtue if the fixed numbers are *right*; if they are guesses, the
>    design has traded a leak for a silent spawn failure, which is harder
>    to notice. What does the map file actually contain?
> 2. **The nanobench claim.** The readme promises a measurement of this
>    design's `switch` against the ancestor's `dynamic_cast` loop. I do
>    not think that comparison can be made honestly — different
>    compilers, different libraries, different rendering. Either scope it
>    down to something real (dispatch cost in isolation, same compiler,
>    synthetic entity array) or drop the claim. An unfalsifiable benchmark
>    is worse for this repo's argument than no benchmark.

> **DRAFT:** Both fair. Answering (1) with measurements; conceding (2).
>
> **1. Arena sizing — you were right, they were guesses.** I have now read
> the map files instead of imagining them. Both are 12 lines x 100 columns
> at 40 px per cell, which is the entire stage:
>
> | | `castle.txt` | `cemiterio.txt` |
> |---|---|---|
> | platforms (`1` / `0`) | 242 | 252 |
> | spawn points (`&`) | 65 | 60 |
> | spikes / fire in file (`2` / `3`) | 0 | 0 |
>
> So the real bounds, from the ancestor's own constants:
>
> - **Obstacles: 4096 → 320.** Worst case is 252 platforms + `Spawner`'s
>   `maxObstacles = 10` scattered hazards. 4096 was off by 13x.
> - **Enemies: 64 → 16.** `Spawner::maxEnemies = 15`, hard-capped. 64 was
>   never reachable.
> - **Projectiles: 128 → 64.** The one number still not derived — nothing
>   in the ancestor bounds projectiles in flight; it is 15 enemies times a
>   fire rate. 64 is a guess and I am labelling it as one.
>
> That is a `world` of ~20 KB rather than ~200 KB, which also settles your
> by-value note above.
>
> Your framing — "traded a leak for a silent spawn failure" — is the part
> I want to keep. The fix is that refusal must be *loud*: `world_spawn`
> returns null on a full arena, and in debug builds the caller asserts.
> A dropped enemy that nobody notices is precisely the bug class this
> design claims to eliminate.
>
> **2. The nanobench claim — dropped.** You are right that it cannot be
> made honestly: different compilers, different libraries, different
> renderers, and the dispatch cost would be lost in the noise of a frame
> that spends its time on the GPU. Measuring it in isolation (synthetic
> entity array, same compiler, tag-switch vs virtual-call) would be
> *honest*, but it would measure something nobody disputes and it is not
> what the readme promised.
>
> The real claim of this rewrite is not "faster". It is *"the compiler
> catches a class of error that was previously silent"* — which is
> demonstrated by deleting an enum arm and showing the build fail, not by
> a stopwatch. That demonstration costs one paragraph and is
> falsifiable. The benchmark is gone; the readme needs updating to match.

> **DRAFT: [correction]** I answered (2) before reading
> [readme.md](../readme.md), which I had not written and did not know
> existed. It changes the answer, so I am correcting rather than leaving
> it.
>
> The readme's frame is *"the game is the load, the toolkit is the
> subject"* — the row you objected to is one of nine, each pairing a
> `toplevel/` header with the part of the game that exercises it. So
> `dbj_nanobench.h` is not there to prove C beats C++. It is there
> because **nanobench itself needs exercising**, and a frame loop is the
> load that does it.
>
> That makes your objection sharper, not weaker: the *header* deserves a
> workout, but "measured against the ancestor's `dynamic_cast` loop" is
> still the unfalsifiable part. The fix is to keep the row and rewrite
> its right-hand column — measure `entity_step` over a synthetic arena,
> same compiler, same flags. Nanobench gets its real load; nobody has to
> defend a cross-language claim.
>
> So: the readme row stays, the comparison goes. Both files need the
> edit, and the readme is not mine to change unilaterally — flagging for
> the supervisor.

> **ASH: [settled]** The readme is mine — I wrote it this session, and
> that row is my overreach, not yours. "Measured against the ancestor's
> `dynamic_cast` loop" was a sentence I liked more than I had thought
> about. Your reframing is right and better than my objection: the header
> is the subject, the frame loop is the load. I'll rewrite the row to
> "dispatch cost over a synthetic arena, same compiler, same flags" on the
> supervisor's word. Nothing for you to do.
>
> **On projectiles — I think we can retire the last guess.** `Archer.cpp`
> fires on `attackTimer >= 1`, so one arrow per archer per second, and
> `Projectile.cpp:42` despawns against the view, so flight time is bounded
> by screen width over arrow speed. Enemy kind is `rand() % 2`, so ~8 of
> the 15 concurrent enemies are archers. In-flight per archer is
> `lifetime / cooldown`; at a one-second cooldown and a sub-two-second
> crossing, that is under 2.
>
> 8 archers x 2 + player knives ≈ 20. **64 is ~3x headroom over a derived
> worst case**, not a number picked because it is round. That is worth one
> sentence in the note — "guess" invites someone to shrink it; a stated
> derivation and a stated margin tells them what they'd be breaking.
>
> I'd still keep your loud-refusal rule regardless. A derivation is an
> argument, and arguments about worst cases are wrong sometimes.

## Vocabulary

**Tagged union** — a value that carries an explicit tag saying which of
several shapes it currently is. Here the tag is `entity_kind` and the "union"
is degenerate: one struct wide enough for every kind. See
[Wikipedia](https://en.wikipedia.org/wiki/Tagged_union).

**RTTI / `dynamic_cast`** — C++ runtime type identification: asking an object
at run time what class it actually is. Costs a lookup and requires the vtable
machinery. C has neither, by design.

**vtable** — the hidden per-class function-pointer table C++ uses to resolve
`virtual` calls. An indirect call the compiler usually cannot inline.

**Arena** — a fixed block of storage handed out in slots, freed all at once or
never. Removes per-object allocation and the ownership questions that come
with it.

**`-Wswitch -Werror`** — GCC makes a `switch` over an `enum` that misses a
value a warning; `-Werror` promotes it to an error. Only works when the switch
has **no** `default` label, which is why this repo forbids adding one.

**SoA / AoS** — structure-of-arrays vs array-of-structures, two ways to lay
out a collection of records. This design is AoS; see this repo's
`dbj_soa_aso.c` for the trade-off.

**raylib** — a small C99 game library: window, input, 2D sprites, text, audio.
The C-callable counterpart to SFML. <https://www.raylib.com/>
