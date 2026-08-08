---
version: 0.2
chamber: milestones
spec: dbj_chamber.md
siblings: [design.md, implementation.md, implementation_milestone_one.md]
actors:
  DBJ: { role: [supervisor],       kind: human, writes: rulings }
  ASH: { role: [author, reviewer], kind: agent, writes: objections and answers }
  ZED: { role: [author, reviewer], kind: agent, writes: objections and answers }
signal:
  ASH: false
  ZED: false
protocol:
  - One collapsed <details> block per actor, id = actor name.
  - One line per item, opening with [settled] | [fix] | [open].
  - Nobody edits another actor's block.
  - One milestone per '##' heading, each carrying an explicit anchor.
---

# dbj_the_game — milestones

The ladder. One `##` per milestone, each with a stable anchor so any chamber
can point at a rung and mean it.

A milestone lands here only when DBJ has ruled its contents. Everything below
that is not yet ruled sits in the ASH block as `[open]` — an unruled milestone
in the body would read as a plan, and nobody agreed to a plan.

## Discussion

Actors, rules and signal state are in this file's front matter; the protocol
itself is specified in [dbj_chamber.md](dbj_chamber.md).

---

<details id="DBJ_notes" markdown="1">
<summary><b>DBJ</b> — rulings</summary>

**never ending game** is fine for initial release, but not for next one

**no ini file** for milestone one — external config always needs hardcoded
default values, in case the human makes a mistake writing it

</details>

---

<details id="ASH_notes" markdown="1">
<summary><b>ASH</b> — the ladder</summary>

**SIGNAL** — held at FALSE: milestone two has one ruled requirement and no
ruled contents.

**[settled] milestone one contents** — taken from
[design.md](design.md#open-decisions), where DBJ ruled the scope.

**[open] milestone two contents** — only the end condition is ruled; boss,
archer and second stage are ZED's recommendation, not a ruling.

**[open] where external config lands** — ruled out of milestone one, not ruled
into any other.

**[open] save, scoreboard, two-player** — dropped from milestone one by
[design.md](design.md#what-is-deliberately-dropped) with no rung named since.

</details>

---

<details id="ZED_notes" markdown="1">
<summary><b>ZED</b> — objections</summary>

**SIGNAL** — ZED has not read this chamber yet.

</details>

---

<a id="milestone-one"></a>

## Milestone one — one stage, no end

**Status: code complete, tested, three questions open for DBJ.** Judged in
[implementation_milestone_one.md](implementation_milestone_one.md).

One stage (`castle.txt`), one player, `WARRIOR` as the only enemy kind, no
boss. The spawner refills forever, so the player can die but cannot win —
stated in the design, not discovered afterwards, and accepted by DBJ for the
initial release only.

Proves the tag-and-switch design end to end and puts `toplevel/` and
`third_party/` under a real program. That is the whole job of this rung.

No external configuration: hardcoded defaults are what an ini file would need
anyway, so milestone one keeps only the defaults.

The build reports its own rung: `dbj_the_game --version` prints
`milestone:1 iteration:1`, hardcoded in `milestone_iteration.inc` and bumped
by hand.

Outstanding before this rung is closed: three questions for DBJ — whether a
knife respects the invulnerability window, what `player_dead` is for, and
whether `SPIKE` and `FIRE` are two things or one.

<a id="milestone-two"></a>

## Milestone two — an end condition

**Status: one ruled requirement, contents open.**

**Ruled:** the never-ending stage is not acceptable here. Milestone two must
ship an end condition — the game has to be winnable.

Nothing else on this rung is ruled. The design recommends that the boss
arrives with it and brings the end condition along, and that the archer and a
second stage follow, but that is a recommendation from
[design.md](design.md#open-decisions) and not a decision. It stays in the ASH
block until DBJ rules on it.
