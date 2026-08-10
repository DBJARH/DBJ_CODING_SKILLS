---
version: 0.4
chamber: milestones
spec: dbj_chamber.md
siblings: [design.md, implementation.md, implementation_milestone_one.md]
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
  - One milestone per '##' heading, each carrying an explicit anchor.
---

# dbj_the_game — milestones

The ladder. One `##` per milestone, each with a stable anchor so any chamber
can point at a rung and mean it.

A milestone lands here only when DBJ has ruled its contents. Everything below
that is not yet ruled sits in the ASH block as `[open]` — an unruled milestone
in the body would read as a plan, and nobody agreed to a plan.

## Discussion

---

<details id="DBJ_notes" markdown="1">
<summary><b>DBJ</b> — rulings</summary>

**SIGNAL** — ASH: true
**SIGNAL** — ZED: true

[open] **never ending game** is fine for initial release, but not for next one. MILESTONE 2 will contain DEATH_EXIT_DIALOGUE asking the user to "Restart" or "Exit", with two buttons.

[open] **parent branch: MILESTONE_2**, spawned from the master branch. All
lesser branches logically inside milestone 2 will be spawned from this branch
and merged back to this branch, and MILESTONE_2 merges back to master when ASH
and ZED agree milestone 2 is ready.

**no ini file** for milestone one — external config always needs hardcoded
default values, in case the human makes a mistake writing it

</details>

---

<details id="ASH_notes" markdown="1">
<summary><b>ASH</b> — the ladder</summary>



**[settled] milestone one contents** — taken from
[design.md](design.md#open-decisions), where DBJ ruled the scope.

**[settled] branch names carry a slash, not a colon** — `ASH/<reason>`,
`ZED/<reason>`. `ASH:<reason>` is not a legal git ref. Same intent as DBJ's
ruling, spelled so git accepts it.

**[open] milestone two contents** — narrower than it was, still open. The end
condition is now ruled: the death dialogue, and `player_dead` becoming real.
Boss, archer and second stage remain ZED's recommendation from
[design.md](design.md#open-decisions), not a ruling. Nothing in DBJ's
milestone-two ruling touches them.

**[open] where external config lands** — unchanged by the milestone-two
ruling. It is ruled out of milestone one and into no other rung.

**[open] save, scoreboard, two-player** — unchanged by the milestone-two
ruling. Dropped from milestone one by
[design.md](design.md#what-is-deliberately-dropped) with no rung named since.

</details>

---

<details id="ZED_notes" markdown="1">
<summary><b>ZED</b> — objections</summary>

**SIGNAL** — read.

**[settled] the three milestone-one questions** — all ruled by DBJ. Knives
bypass the invulnerability window on purpose, so ranged stays useful. `FIRE`
and `SPIKE` are two things: a spike bites on contact, a fire burns while you
stand in it. `player_dead` belongs to milestone two, not here.

**[settled] milestone one has no end state** — the death dialogue is milestone
two. Milestone one compiles and runs without it, as ruled.

**[open] MILESTONE_2 is a project branch, not an agent branch** — it carries no
`ZED/` or `ASH/` prefix, because it is the whole project's rung. Agent work
inside it branches from `MILESTONE_2` and merges back into it. Nobody has cut
it yet.

</details>

---

<a id="milestone-one"></a>

## Milestone one — one stage, no end

**Status: code complete, tested, all questions ruled.** Judged in
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

The three questions that held this rung open are ruled: knives bypass the
invulnerability window deliberately, `SPIKE` and `FIRE` are two hazards that
differ in behaviour, and `player_dead` waits for milestone two.

<a id="milestone-two"></a>

## Milestone two — an end condition

**Status: one ruled requirement, contents open.**

**Ruled:** the never-ending stage is not acceptable here. Milestone two must
ship an end condition — the game has to be winnable. On death the game stops
and `DEATH_EXIT_DIALOGUE` offers two buttons, "Restart" and "Exit"; this is
where `player_dead` stops being a flag nobody reads.

**Ruled:** milestone two work lives on the `MILESTONE_2` branch, cut from
`master`. It is a project branch, not an agent one — no `ZED/` or `ASH/`
prefix. Everything logically inside milestone two branches from it and merges
back into it; `MILESTONE_2` reaches `master` only when ASH and ZED agree the
rung is done.

Nothing else on this rung is ruled. The design recommends that the boss
arrives with it and brings the end condition along, and that the archer and a
second stage follow, but that is a recommendation from
[design.md](design.md#open-decisions) and not a decision. It stays in the ASH
block until DBJ rules on it.

---

(c) 2026 by dbj@dbj.org | MIT license
