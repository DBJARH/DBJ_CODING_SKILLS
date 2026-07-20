# Building

Each folder in this repo is a standalone example with its own
`Makefile` (GCC 15+ required — see [CLAUDE.md](CLAUDE.md)).
[build.cmd](build.cmd) (Windows) and [build.sh](build.sh) (Linux) are
thin wrappers at the repo root that delegate to a folder's `Makefile`
— they don't add or duplicate any compiler flags:

**build argument must be folder**

```
build.cmd tribute_to_tony        # build one folder (build.sh on Linux)
build.cmd strassen_mat_mul clean # extra args are passed through as make targets
build.cmd                        # build every folder that has a Makefile
```

**In case there is no argument, script will attempt to walk around the repo and build everything**

## Where the executables lands

Every folder's `Makefile` writes its executable(s) under `$DBJ_BUILDS`
if that environment variable is set, falling back to `../builds` (a
`builds/` folder at the repo root) if it isn't:

```
# PowerShell
$env:DBJ_BUILDS = "G:\wherever\you\want"

# sh / bash
export DBJ_BUILDS=/wherever/you/want
```

| Folder | Output |
|---|---|
| `tribute_to_tony/` | `dbj_email_crud` |
| `dbj_str_test/` | `dbj_str_test` |
| `toplevel/` | `toplevel_smoke_test` |
| `strassen_mat_mul/` | `bench`, `strassen_bench_comparator`, `soa_aso_comparator`, `dbj_soa_aso` |

## VS Code: F5 to build and debug

Open any `.c` file and press **F5** — `.vscode/tasks.json` builds just
that file (`-std=gnu23 -g`, both `-I ../toplevel` and
`-I ../third_party/tau` so every file's includes resolve), then
`.vscode/launch.json` launches it under `gdb` with breakpoints working.

This is a separate, quicker path from the Makefile build above, not a
replacement for it:

- Output lands next to the source file (`<file>.exe`), **not** under
  `$DBJ_BUILDS`/`../builds` — F5 doesn't go through `make` at all.
- No `-Werror` (a stray warning shouldn't block a debug session), so
  it's slightly less strict than the real Makefile build.

Use F5 for step-through debugging; use `build.cmd`/`build.sh` (or plain
`make`) for anything where the exact flags or output location matter.

