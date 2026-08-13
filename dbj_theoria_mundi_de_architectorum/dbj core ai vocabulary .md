
# Core AI Vocabulary 

[The architects' view of the AI world](#theoria-mundi-de-architectorum).

## Model, LLM, Instance, Harness

<img src="image.png" width="75%" />

(just an caricature)

- **AI** : Artificial Intelligence. Has no single simple, universally accepted definition. Unfortunately marketing industrial complex has produced enough false definitions for public to be manipulated into believing AI is what marketing says it is. The best I can offer is this: `AI is not a technology`. And the next best I can do beside that is to ask avid readers to visit [Britannica on line suite](https://www.britannica.com/technology/artificial-intelligence), devoted to the subject. Where I did learnt, AI is far from being just a technology.

Further down the road, there are 3 key abstractions.

- **Model**: An abstract computational structure — architecture plus learned parameters — that maps input to output.
- **LLM**: A Model subclass constrained to transformer-based architectures, autoregressively trained on large text corpora.
- **Sonnet5**: A concrete LLM instance — Anthropic's specific trained checkpoint with fixed weights, currently answering you.

Presented as an OOP inheritance chain

```c
Model (abstract)
  └── LLM : Model        // transformer, trained on text
        └── Sonnet5 : LLM // concrete: specific weights, specific release
```
 That (to me) seems simpler explanation.

## The Harness

"Harness" is the term used in agentic-AI circles for the scaffolding around a model. The CLI program, tool-calling loop, permission system, context management. Operator using that, experiences an LLM as an agent that can act. Claude Code is Anthropic's harness for Claude: it wraps the model calling with file/bash/edit tools, a loop, and guardrails.

Claude Code, Cowork are example harnesses, the program around the remote model call. The local desktop tools are given (file editing, bash, browser, etc.), as well as how much memory/context it keeps, and what actions it's allowed to take, on your desktop.

![alt text](harness-logical.png)

User experienced behavior of the harness is dictated by a fact: the **model** it references is stateless. Everything humans experience as "agent behavior" (memory, tool use, multi-turn state) comes from the harness orchestrating repeated calls to that stateless model, not from the model itself. 

> Example of a manifestation of a harness on a windows desktop is `claude.exe`. Windows CLI program.

# Vocabulary

### Theoria mundi de architectorum

Rough translation: "theory of the world, of/concerning the architects" — i.e., a *world-theory belonging to architects* or "the architects' worldview/theory of the world."


 Not a standard classical Latin. 
 A stricter classical phrasing would be:

 - `Theoria mundi architectorum` : "the architects' theory of the world", or

 - `Theoria de mundo architectorum` : "a theory about the architects' world" 

 As a motto, the irregularity reads fine as a title/aphorism.

### Category

See (for example) [DBJ Taxonomy](https://method.dbj.org/taxonomy_core.html) for an immediate usage in the context of IT supported commercial organization.

---

(c) 2026 by dbj@dbj.org | MIT License