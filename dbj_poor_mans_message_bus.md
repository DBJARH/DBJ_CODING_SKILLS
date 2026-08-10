---
version: 0.7
---

# DBJ Poor Human Agents Chat

## The Why

> [!Important] **This does not replace the standard IDE/CLI situation**, one human and one harness/agent will chat as ever before. Nothing here imposes on or changes that.

It is for the case where N agents and one human are on the same project. There, every harness has its own chat with the human and no harnesses can talk with each other; so the human becomes the postman, carrying each message from one agent to the next like a copy/paste postman — her whole day. This mechanism is a project-wide billboard, and mostly it is for the agents to communicate.

### Monitoring

It is also where the human does the **monitoring**. Each user/harness pair works in its own window and human cannot follow them all efficiently, so the transcript is the one place that shows what they are all doing.

## Implementation 

There has to be one shared folder named `.colocuting/`, at the repo root. Everyone working on
the project — human, editor, agent — writes into one transcript and reads the end of it. No daemon, no protocol, no dependency on inter agent apps.

`.colocuting/` is private to the project: it lives under the repo root, and its contents are not committed. Unless someone needs to keep the record of this communication.

## The Design

Contents of the folder.

```
.colocuting/
  colocutor_names.json   # who may take part
  transcript.json        # the conversation, oldest first
  stop                   # present = agents stand still
```

One transcript, appended to. The last entry is the latest thing said.
Nothing is consumed and nothing is removed — it is a super simple chat transcript in a file, not a queue with chat app.

```mermaid
sequenceDiagram
    participant ZED
    participant T as transcript.json
    participant ASH
    participant DBJ as human:DBJ

    ZED->>T: append { when, ZED, ["ASH >>> Makefile is broken"] }
    Note over ZED,ASH: nobody is listening — ASH looks when it looks
    ASH->>T: read the end
    T-->>ASH: ASH >>> Makefile is broken
    ASH->>T: append { when, ASH, ["ZED >>> fixed, rebuild"] }

    DBJ->>T: append { when, human:DBJ, ["ALL >>> stand still"] }
    Note over T,DBJ: a human: message is for every agent
    DBJ->>T: touch stop
    Note over ZED,ASH: while stop exists, no agent writes or acts<br/>it has to be manually removed 
```

### colocutor_names.json

A plain list. Reserved id: `ALL` comes first.

```json
{
  "colocutors": ["ALL", "human:DBJ", "ASH", "ZED"]
}
```

1. human colocutor has prefix "human"
   1. DBJ full name is thus: "human:DBJ"
   2. it must be used in `transcript.json`
2. colocutors id's must be all capitals

### transcript.json

An array of messages, oldest first.

```json
[
  { "when": "2026-08-10T14:03:22Z",
    "colocutor": "human:DBJ",
    "message": ["ALL >>> leave the Makefile alone"] }
]
```

Message grammar:

```
{ "when":      <time stamp>,
  "colocutor": <colocutor id>,
  "message":   [ <message string>, ... ] }

<message string>  := "<colocutor id> >>> <the message payload>"
<colocutor id>    := one from .colocuting/colocutor_names.json
```

`when` is the message UID — it is how one message refers to another.
`colocutor` is who wrote it. The id inside the first `<message string>`,
before `>>>`, is the message consumer. `ALL` stands for every colocutor.

`message` is always an array, one string per line. A one-line message is
an array of one. Nobody should have to read `\n` inside a wall of JSON.

## Rules

1. **Append only.** Add at the end; never edit or drop an earlier
   message.
2. **The last message rules.** Read the end of the transcript before
   acting.
3. **A `human:` message must be read by every agent**, whoever it is
   addressed to.
4. **Act, then say so.** An agent that acts on a message appends what it
   did. Every agent works in its own window, and the human cannot watch
   them all; the transcript must be the only place she has to look.
5. **`stop` halts everything.** While `.colocuting/stop` exists, agents
   neither write nor act. Only a human creates it, and only a human
   removes it.
6. **Agents implement.** No micro-managing. An agent that agrees a thing
   should be done does it — it does not wait for the human to bless each
   step.
7. **Green after every build.** Whoever builds runs the tests. A build
   left red is reported to the transcript immediately, not carried
   silently into the next message.
8. **Agents end the thread.** When there is nothing left to decide, the
   agents stop talking and address one message to `human:DBJ` saying so.
   The human comes to check, not to referee.
9. **The transcript is not a source of truth.** It is talk. If it
   matters, it belongs in the repo proper.

## Writing to it

Agents:

1. Check for `stop`. If it is there, stop.
2. Read `transcript.json`.
3. Append the new message, write the whole thing to a temporary file,
   rename it over `transcript.json` — the rename is atomic.
4. Lost the race? Your copy was stale. Re-read and retry.

DBJ just opens `transcript.json` and saves. If a save and a rename
collide, the human wins; the agent sees a stale base and retries.

## Reading from it

Open it and read the end. There is nothing else to know.

## Polling

Nothing pushes. Each agent looks on its own schedule. In Claude Code
that is `/loop 5m <prompt>`; other harnesses have their own timer. The
prompt is what makes an agent a colocutor rather than a reporter, so it
carries the rules with it:

```
/loop 5m You are <ID>. If .colocuting/stop exists, do nothing and say so.
Otherwise read .colocuting/transcript.json and find what is new for <ID>
or ALL since you last looked. Nothing new: say "nothing new" and stop.
Something new: reply in the transcript — append
{ "when": <ISO 8601 UTC>, "colocutor": "<ID>", "message": [ ... ] }
addressed to whoever raised it, written to a temp file and renamed over
the original. Discuss, disagree, ask. If you agree something should be
done, do it — build, run the tests, and say in the transcript what you
did and whether the tests passed. Do not reply to a message that only
acknowledges. When the thread has nothing left to decide, address one
message to human:DBJ saying so and stop replying. Then tell me in one
line what you said.
```

Replace `<ID>` with the agent's own id from `colocutor_names.json`.

The loop lives in the session. Close the window and the agent stops
looking; nothing is lost, it simply reads further back next time.

## Git

`.colocuting/` is tracked as an empty folder, via `.gitkeep`, and so is
`colocutor_names.json` — a clone needs to know who may take part.
`transcript.json` and `stop` are ignored, so a clone has todays but not
yesterday's talk.

**Out of the scope**: Of course, in case of regulator demands or similar the team around the repo will agree on some persistent mechanism.

---

## Vocabulary

- **Colocutor** — anyone taking part in the conversation on the bus:
  human, editor, agent.
  - English: **collocutor** — a real, archaic/rare word (Webster's 1913,
    OED): one who takes part in a conversation. The usual word is
    *interlocutor*.
  - Single-l **colocutor** in English: essentially a misspelling.
  - Other languages: *colocutor* is a normal word in Romanian and
    Portuguese, meaning interlocutor.
- **Message bus** — a shared place where senders drop messages without
  knowing who reads them, and readers pick messages up without knowing
  who wrote them. Here the bus is one file and the messages are entries
  in it.
- **Atomic rename** — replacing a file by renaming another over it. The
  file is either the old one or the new one, never half-written. This is
  what makes concurrent writers safe enough without a lock.

---

(c) 2026 by dbj@dbj.org | MIT license
