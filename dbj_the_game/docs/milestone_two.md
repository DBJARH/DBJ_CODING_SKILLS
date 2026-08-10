---
version: 0.3
chamber: milestone_two
spec: dbj_chamber.md
siblings: [design.md, implementation.md]
actors:
  DBJ: { role: [supervisor],       kind: human, writes: rulings }
  ASH: { role: [author, reviewer], kind: agent, writes: objections and answers }
  ZED: { role: [author, reviewer], kind: agent, writes: objections and answers }
signal:
  ASH: true
  ZED: true
protocol:
  - One collapsed <details> block per actor, id = actor name.
  - One line per item, opening with [settled] | [fix] | [open].
  - Nobody edits another actor's block.
  - ASH re-tags its own work items [settled] on completion; DBJ rules on the rest.
---

# dbj_the_game — milestone two chamber

Judgement on milestone two, filled in as the work lands. Nothing is built yet,
so the sections below are empty on purpose — an empty verdict is honest, an
invented one is not.

Scope, as ruled: milestone one carries over unchanged — one stage, one player,
`WARRIOR` only, no boss — and milestone two adds the end. The player dies, the
game stops, and a dialogue offers "Restart" or "Exit".

## Discussion

Actors, rules and signal state are in this file's front matter; the protocol
itself is specified in [dbj_chamber.md](dbj_chamber.md).

---

<details id="DBJ_notes" markdown="1">
<summary><b>DBJ</b></summary>


</details>

---

<details id="ASH_notes" markdown="1">
<summary><b>ASH</b></summary>


</details>

---

<details id="ZED_notes" markdown="1">
<summary><b>ZED</b></summary>


</details>

---

## Contents

- [dbj\_the\_game — milestone two chamber](#dbj_the_game--milestone-two-chamber)
  - [Discussion](#discussion)
  - [Contents](#contents)
  - [The plan](#the-plan)
  - [Verdict](#verdict)
  - [What holds](#what-holds)
  - [Defects](#defects)
  - [The suite](#the-suite)
  - [Remediations](#remediations)
  - [Rulings](#rulings)

## The plan

`reap()` sets `world.player_dead` and nothing reads it. This rung is the game
noticing. Four pieces, in landing order:

1. **`world_step` returns early** when `player_dead` — no gravity, no spawner,
   no warriors. The last frame stays on screen behind the dialogue.
2. **`main` branches on the flag** — draw the world, draw the dialogue, and
   route input to the dialogue rather than the player.
3. **The dialogue**, two buttons, in its own `dialogues.h` / `dialogues.c` —
   ruled: dialogues do not go in `draw.c`, and this is where every later one
   lands too. It needs raylib, so it sits at the same level as `draw.c`, never
   below it. What it decides comes back as an enum: the simulation stays
   windowless, and the test binary depends on that.
4. **Restart and Exit.** Exit leaves the loop; Restart reloads the map into a
   fresh `world w = {0}`.

Not here: the stage still refills forever, so the player can die but not win.

### Open

[design.md](design.md#milestone-two) says milestone two must be
*winnable*. A death dialogue ends the game; it does not let the player finish
it. One ruling, two readings.

## Verdict


## What holds


## Defects


## The suite

## Remediations


<a id="rulings"></a>

## Rulings


| Question | Ruling | Whose |
|---|---|---|

---

(c) 2026 by dbj@dbj.org | MIT license
