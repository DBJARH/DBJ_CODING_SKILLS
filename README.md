# DBJ Coding Skills

A collection of small, focused C proof-of-concepts exploring coding
concepts and comparing approaches — currently centered on discriminated
unions / tagged records versus OOP-style dispatch.

## Contents

- [tribute_to_tony/](tribute_to_tony/) — the main writeup. Explains
  Hoare's 1966 "Record Handling" concept (tagged unions + exhaustive
  switch), why OOP dropped it, and why it resurfaced in Rust/Swift/Kotlin.
  See [tribute_to_tony/readme.md](tribute_to_tony/readme.md) for the full
  article, [tribute_to_tony/oop_discriminated_union.c](tribute_to_tony/oop_discriminated_union.c)
  for the code, and [tribute_to_tony/code_review.md](tribute_to_tony/code_review.md)
  for an AI-made, human-assisted comparison.
- [not_oop_dbj_improved/](not_oop_dbj_improved/) — the human-improved,
  non-OOP version of the tribute_to_tony example.
- [discarded_vibecode/](discarded_vibecode/) — the original vibe-coded
  (AI-generated) version, kept for comparison.

## License

MIT — see [LICENSE](LICENSE).
