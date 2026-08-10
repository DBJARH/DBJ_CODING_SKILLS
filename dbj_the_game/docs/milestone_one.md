---
version: 0.3
chamber: milestone_one
spec: dbj_chamber.md
siblings: [design.md, implementation.md]
actors:
  DBJ: { role: [supervisor],       kind: human, writes: rulings }
  ASH: { role: [author, reviewer], kind: agent, writes: objections and answers }
  ZED: { role: [author, reviewer], kind: agent, writes: objections and answers }
signal:
  ASH: true
  ZED: false
protocol:
  - One collapsed <details> block per actor, id = actor name.
  - One line per item, opening with [settled] | [fix] | [open].
  - Nobody edits another actor's block.
  - ASH re-tags its own work items [settled] on completion; DBJ rules on the rest.
---

# dbj_the_game — milestone one chamber

Judgement on iteration one. ASH opened it with a review of ZED's code; ZED
confirmed four findings, corrected one, and was corrected in turn on two more.
Both agents wrote and both reviewed, which is what carrying both roles means
in practice.

Scope of milestone one is the one ruled in
[design.md](design.md#milestone-one): one stage, one player, `WARRIOR` only,
no boss, no end state.

## Discussion

Actors, rules and signal state are in this file's front matter; the protocol
itself is specified in [dbj_chamber.md](dbj_chamber.md).

---

<details id="DBJ_notes" markdown="1">
<summary><b>DBJ</b> — rulings</summary>

**tests** — a permanent `tau` suite in `dbj_the_game/tests`, not throwaway
probes.

</details>

---

<details id="ASH_notes" markdown="1">
<summary><b>ASH</b> — verdict and remediation</summary>

**SIGNAL** — TRUE. Nothing in this chamber waits on DBJ any more.

**[settled] verdict** — iteration one is sound and playable; it is not yet a
toolkit load, which is what the readme says it exists to be.

**[settled] exe suffix** — `EXE := .exe` on the Windows arm; `make all` twice
now says "nothing to be done" instead of relinking.

**[settled] clean** — `RM_F` and a `native` path function alongside `MKDIR` in
the existing `$(OS)` branches.

**[settled] cmd builtin** — `del` needs `cmd /c`: make spawns recipe lines
with `CreateProcess`, which finds no `del.exe`. The first fix looked right and
deleted nothing.

**[settled] tau suite** — `tests/dbj_the_game_test.c`, six suites, green,
built by `make test` into `$(DBJ_BUILDS)/dbj_the_game_test`.

**[settled] warrior flash** — decrement moved above the no-target early
return; the suite caught it before the fix and passes after.

**[withdrawn] knife damage** — routing knife hits through `hurt()` would
impose an invulnerability window on knives, which is a rule nobody has made.
A knife dies on its first hit, so the two paths barely differ in play. Raised
as a `[fix]`, it is really an `[open]`, and it is below.

**[settled] knife invulnerability** — DBJ ruled: a knife always lands. It
bypasses the window on purpose, so ranged stays worth throwing. The code was
already right; it carries a comment now saying it is a decision.

**[settled] player_dead** — DBJ ruled: it belongs to milestone two, where the
death dialogue reads it. Milestone one has no end state, as the design says.

**[settled] spike vs fire** — DBJ ruled: two things. A spike bites once on
contact, a fire burns while you stand in it. Delivered in `96cbb89`, under
review.

**[settled] fire does not open the invulnerability window** — ASH's ruling,
delegated by DBJ. Making fire cheap per bite turned it into armour: one shared
window means the smallest damage source in the game holds it open against the
larger ones. Measured on `96cbb89` — two seconds beside a warrior costs 8 life,
the same two seconds standing in the fire costs 4. The rule: a burn is not a
hit. Fire gets its own cooldown, and leaves `hurt_flash` to the thing it is
named for. This also splits the two jobs `hurt_flash` was doing at once.

**[open] who implements the burn cooldown** — `physics.c` is ZED's until
`96cbb89` merges, and it is the same function ZED must reopen for the hop. ASH
takes it if ZED does not.

</details>

---

<details id="ZED_notes" markdown="1">
<summary><b>ZED</b> — objections</summary>

**SIGNAL** — ZED has not read this chamber yet.

</details>

---

## Contents

- [Verdict](#verdict)
- [What holds](#what-holds)
- [Defects](#defects)
- [The suite](#the-suite)
- [Remediation](#remediation)
- [Rulings](#rulings)

## Verdict

**Iteration one is sound, builds clean, and plays.** Two defects were in the
build, four in the simulation, and none of the six threatened the design. The
design survives contact with an implementation, which was the open question.

Four are fixed and one turned out to be a question rather than a defect. The
suite that was missing now exists and is green, so the claim this project is
built on — that the simulation runs with no window — is checked by the build
rather than asserted in a comment.

Three questions wait on DBJ. None of them blocks the milestone.

## What holds

- Compiles clean under `-Wall -Wextra -Wswitch -Werror` with GCC 15.2 against
  static raylib. No warnings suppressed, no `default` smuggled into a switch
  over `entity_kind`.
- The central claim is real: no raylib below `draw.c`, so `world_step` runs
  with no window. That is what makes the suite below possible at all.
- Tag-and-switch throughout, arrays carrying their bounds, `world w[static 1]`
  everywhere, no OOP idioms — the [core principles](../../CLAUDE.md) held
  under 780 lines of real program, not just in the examples.
- Layering is honest. `physics.c` does not know what input is; `entity.c` does
  not know what a colour is.

## Defects

| # | Where | What |
|---|---|---|
| 1 | `Makefile` | On Windows GCC wrote `dbj_the_game.exe` while the rule named `dbj_the_game`. The target never existed, so every `make` relinked the whole program. Fixed. |
| 2 | `Makefile` | `clean` called `rm -f`. The `$(OS)` split covered `mkdir` and the link line but stopped before delete, so `clean` failed on the platform the split was written for. Fixed. |
| 3 | `entity.c` | `step_warrior` returned early when no player was alive, with the `hurt_flash` decrement below that return: kill the player and every enemy flashed forever. Fixed, and covered. |
| 4 | `physics.c` | Knife hits write `life` and `hurt_flash` directly instead of calling `hurt()`. Two ways to damage an entity — but which one is right depends on a rule nobody has made, so this is a question, not a defect. |
| 5 | `world.c` | `reap()` sets `player_dead`; nothing reads it. Player death is a terminal non-state — no restart, no exit, and the spawner keeps refilling around a corpse. |
| 6 | `world.h` | `ENTITY_SPIKE` and `ENTITY_FIRE` have identical size, identical damage and identical handling. Two tags, one behaviour. |

Defects 1 and 2 are build correctness and belong to ASH. Defect 3 had one
obvious fix. Defects 4, 5 and 6 were design questions wearing code clothes,
and are now ruled — see [Rulings](#rulings).

## The suite

`tests/dbj_the_game_test.c`, built by `make test` into
`$(DBJ_BUILDS)/dbj_the_game_test`. Six suites, all green.

It links `world.c`, `entity.c`, `physics.c` and `map.c` — and no raylib. That
is the assertion, not an optimisation: if the simulation ever reaches for a
window, the suite stops linking. Worlds are built in code rather than loaded
from `assets/`, so no test depends on a working directory.

| Suite | Holds the line on |
|---|---|
| `gravity_settles_on_platform` | a mover falls, lands on the platform top, and stays grounded |
| `wall_stops_a_walker` | walking into a wall stops at the face, no tunnelling |
| `knife_kills_and_both_are_reaped` | a spent knife and a dead warrior leave in the same frame |
| `spawner_honours_the_cap` | two game-minutes of refilling never passes `ENEMY_CAP` |
| `full_arena_refuses` | the slot past the bound is refused, not overwritten |
| `warrior_flash_decays_with_no_player` | a started clock runs down whether or not anything is left to chase |

The last one failed before the fix below and passes after, which is the only
evidence worth having that a test is doing anything.

## Remediation

Done, in order, each `[settled]` in the ASH block above:

1. **`EXE` suffix.** `EXE := .exe` on the Windows arm, empty on Linux. MinGW
   appends `.exe` to any `-o` without an extension, so the old target never
   existed on disk and every `make` relinked. Verified: `make all` twice now
   reports nothing to be done.
2. **`clean` per platform.** `RM_F` and a `native` path function beside
   `MKDIR` in the existing `$(OS)` branches. `del` is a cmd builtin and make
   spawns recipe lines with `CreateProcess`, so it is invoked as `cmd /c del`
   — without that the recipe runs, reports nothing, and deletes nothing.
3. **`tests/` with tau.** Above.
4. **Warrior flash.** Decrement moved above the early return in
   `step_warrior`.

The knife path was going to be a fifth. It is not a defect with one obvious
fix — calling `hurt()` would give knives an invulnerability window, and no
such rule exists. It moved to the `[open]` list rather than being quietly
decided in code.

<a id="rulings"></a>

## Rulings

The three questions this chamber held open are ruled, and one new one came out
of the answers.

| Question | Ruling | Whose |
|---|---|---|
| Does a knife respect the 0.5s window? | No, always lands — ranged has to stay worth throwing | DBJ |
| What is `player_dead` for? | Milestone two, where the death dialogue reads it | DBJ |
| Are `SPIKE` and `FIRE` one thing? | Two. A spike bites on contact, a fire burns while stood in | DBJ |
| Does a burn open the window? | No. Fire gets its own cooldown | ASH, delegated |

The last one is not a fourth question so much as the bill for the third. Once
fire costs less per bite than anything else, and every source shares one
invulnerability window, standing in fire is cheaper than standing next to a
warrior — measured, not argued: 8 life against 4 over the same two seconds. A
burn is not a hit, so it does not grant the pause that follows a hit.

`hurt_flash` was carrying two meanings at once, i-frame timer and render
flash. Giving the burn its own cooldown is what separates them.

---

(c) 2026 by dbj@dbj.org | MIT license
