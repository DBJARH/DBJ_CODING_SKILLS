
# observations on [source code of](v1_substandard_claude_design.c)

Implements [RQ01](../top_level_requirements.md#rq01-email-crud-application).

### Human observation

This is a bit weird app design inherited from Claude. Central entities (on of) are for example parameters to email CRUD commands./functions.

### Claude Code Response

That observation is accurate, and I'd push it further. Looking at the actual code, the "weird" part is concrete: EmailCmdParams/EmailCmd (lines 137–147) makes the command's parameter payload the central data structure — create, read, update, del are just anonymous structs nested inside one big union, and the EMAIL_*_CMD macros (151–158) exist purely to paper over how awkward that union is to construct by hand. Compare that to a design where EmailRecord (the actual domain entity — a stored email) is central, and commands are thin wrappers that reference an id plus a delta. Here it's inverted: the command shape drives the design, not the data it's supposed to persist.

That's arguably backwards for a CRUD app — CRUD is normally "data first, verbs are incidental." What you got instead is verb-first, data-incidental, which is a symptom of an LLM pattern-matching "here's a command dispatcher" template rather than modeling the actual email domain.

I'd flesh out line 6 to name this precisely rather than leave it vague ("weird," "on of"). Want me to tighten that sentence into something like:

Central entities are the CRUD command parameter structs (EmailCmd.params.create/read/update/del), not the domain record (EmailRecord) itself — the design is verb-first (commands drive the union) rather than data-first (the email record drives the shape), which is backwards for a CRUD app.