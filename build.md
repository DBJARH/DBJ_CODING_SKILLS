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
| `dbj_str/` | `dbj_str_test` |
| `toplevel/` | `toplevel_smoke_test` |
| `strassen_mat_mul/` | `bench`, `strassen_bench_comparator`, `soa_aso_comparator` |


