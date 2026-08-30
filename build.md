# Building

## `DBJ_CORELIB` first

**Every Makefile in this repo requires `DBJ_CORELIB`**, pointing at
this repo's [corelib/](corelib/) folder. It is checked, not guessed:
an unset variable stops the build with a message. Forward or backward
slashes both work.

```
# PowerShell -- set it permanently, then restart the shell
[Environment]::SetEnvironmentVariable("DBJ_CORELIB", "G:\repos\DBJARH\DBJ_CODING_SKILLS\corelib", "User")

# sh / bash
export DBJ_CORELIB=~/Repositories/DBJARH/DBJ_CODING_SKILLS/corelib
```

Each folder in this repo is a standalone example with its own
`Makefile` (GCC 15+ required — see [CLAUDE.md](CLAUDE.md)).
[build.sh](build.sh) is a thin wrapper at the repo root that delegates
to a folder's `Makefile` — it doesn't add or duplicate any compiler
flags. One script for every platform; on Windows run it from Git Bash:

**build argument must be folder**

```
./build.sh research_and_development/tribute_to_tony
./build.sh research_and_development/strassen_mat_mul clean
./build.sh                        # build every folder that has a Makefile
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

All folders below except `corelib/` live under
`research_and_development/`.

| Folder | Output |
|---|---|
| `corelib/` | `corelib_smoke_test` |
| `tribute_to_tony/` | `dbj_email_crud` |
| `ken_thompson_grep/` | `dbj_grep` |
| `dbj_str_test/` | `dbj_str_test` |
| `dbj_hashmap/` | `dbj_hashmap_smoketest`, `dbj_make_hashmap_smoketest`, `dbj_hashmap_benchmarks` |
| `strassen_mat_mul/` | `bench`, `strassen_bench_comparator`, `soa_aso_comparator`, `dbj_soa_aso` |

## VS Code: F5 to build and debug

Open any `.c` file and press **F5** — `.vscode/tasks.json` builds just
that file (`-std=gnu23 -g`, both `-I ${workspaceFolder}/corelib` and
`-I ${workspaceFolder}/third_party/tau` so every file's includes
resolve), then
`.vscode/launch.json` launches it under `gdb` with breakpoints working.

This is a separate, quicker path from the Makefile build above, not a
replacement for it:

- Output lands next to the source file (`<file>.exe`), **not** under
  `$DBJ_BUILDS`/`../builds` — F5 doesn't go through `make` at all.
- No `-Werror` (a stray warning shouldn't block a debug session), so
  it's slightly less strict than the real Makefile build.

Use F5 for step-through debugging; use `build.sh` (or plain `make`) for
anything where the exact flags or output location matter.

---

(c) 2026 by dbj@dbj.org | MIT license
