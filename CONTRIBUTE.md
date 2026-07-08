

> **Caveat Emptor**: We are enjoying the metapresence of [DBJ Taxonomies](https://method.dbj.org/taxonomy_core.html). Thus we know where are we in the information space with these endeavor. Top category: **Implementation**. Capability: **Development**. In other word: We know what is this all about. And we can explain it to the reader.
>

# How to Build tribute_to_tony

## What you need

- Windows 10 or later
  - Might be not needed. To be confirmed.
  - AFAIK there is no WIN specific code in here.

## Mandated compiler is GCC 15.x or better

- On this machine MinGW-w64 GCC installed at `G:\mingw64`
  (get it from https://winlibs.com — download the latest GCC release, extract to `G:\mingw64`)
    - of course for any other folder preference of yours, you wil change this setup easily

On this machine the CLI check delivers:

```
PS G:\mingw64\bin> gcc --version
gcc.exe (MinGW-W64 x86_64-msvcrt-posix-seh, built by Brecht Sanders, r1) 15.2.0
Copyright (C) 2025 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
```

GCC 15 or better is the *only* supported compiler — Clang, MSVC, and
everything else are explicitly not supported (some code relies on GCC
extensions, e.g. nested functions, that Clang does not implement).
This is enforced at compile time, not just documented: include
[`toplevel/required_compile_time.h`](toplevel/required_compile_time.h)
and any build with Clang or with GCC older than 15 fails immediately
with a `#error`.

---

## IntelliSense warning

After opening the folder, IntelliSense may show false errors (e.g. `uint8_t is not a type name`, `constexpr is undefined`) even though the code compiles fine. This happens because IntelliSense hasn't indexed the GCC headers yet.

Fix: open the Command Palette (**Ctrl+Shift+P**), run **C/C++: Reset IntelliSense Database**, and wait ~15 seconds. The errors will clear.

---

## Option 1 — VS Code (recommended)

1. Open the `g:\about` folder in VS Code (`File > Open Folder`)
   1. Or hwerever is your repo on your machine
2. Press **Ctrl+Shift+B** — this runs the default build task
3. Watch the Terminal VS Code panel at the bottom for output
4. If the build succeeds you will see no errors and the file `tribute_to_tony\tribute_to_tony.exe` will appear

---

## Option 2 — Command line (PowerShell or cmd)

Open a terminal, navigate to this folder, and run:

```
G:\mingw64\bin\gcc -std=c2x -Wall -Wswitch -Werror -o tribute_to_tony tribute_to_tony.c
```

Broken down, each flag means:

| Flag | What it does |
|---|---|
| `-std=c2x` | Use the C23 standard (the latest C; the code uses C23 features) |
| `-Wall` | Turn on all common warnings |
| `-Wswitch` | Warn if a `switch` on an enum is missing a case — this is central to the design |
| `-Werror` | Treat every warning as a hard error so nothing slips through |
| `-o tribute_to_tony` | Name the output executable `tribute_to_tony.exe` |
| `tribute_to_tony.c` | The source file to compile |

---

## Running the program

After a successful build:

```
.\tribute_to_tony.exe
```

You will see a log of CRUD operations (Create, Read, Update, Delete) on a tiny in-memory email store, demonstrating Hoare-style discriminated-union dispatch.

---

## How to debug in VS Code

Debugging requires the **C/C++ extension** by Microsoft and the **gdb** debugger that ships with MinGW-w64.

### 1. Install the extension

Install the [C/C++ extension by Microsoft](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools) from the VS Code Marketplace, or open the Extensions panel (**Ctrl+Shift+X**), search for `C/C++` (publisher: Microsoft), and install it.

### 2. Build with debug symbols

The normal build task (`Ctrl+Shift+B`) strips debug info. For debugging, run the debug build task instead:

- Open the Command Palette (**Ctrl+Shift+P**)
- Type `Tasks: Run Task` and press Enter
- Select **Build tribute_to_tony (debug)**

This adds the `-g` flag, which embeds source-line information in the `.exe` so gdb can map machine instructions back to your C code.

### 3. Set a breakpoint

Click in the gutter (left of the line numbers) in `tribute_to_tony.c` to place a red dot. A good first stop is the `email_dispatch` function or the first `UNWRAP` call in `main`.

### 4. Start the debugger

Press **F5** — or go to **Run > Start Debugging**.

VS Code will automatically rebuild (debug build) then launch `gdb` attached to the `.exe`. Execution pauses at your breakpoint.

### 5. Debugger controls

| Key | Action |
|---|---|
| **F5** | Continue to next breakpoint |
| **F10** | Step over (next line, don't enter functions) |
| **F11** | Step into (enter the called function) |
| **Shift+F11** | Step out (finish current function, return to caller) |
| **F9** | Toggle breakpoint on current line |

### 6. Inspect data

While paused:

- Hover over any variable to see its value
- The **Variables** panel (left sidebar) shows all locals and their fields — expand a struct like `EmailCmd` to see `tag` and `params`
- The **Watch** panel lets you type any expression, e.g. `cmd->tag` or `db.storage[0].id`
- The **Call Stack** panel shows how you got to the current line

### Tip

The whole point of this program is the `switch (cmd->tag)` in `email_dispatch`. Step into it and watch the `tag` value decide which `impl_*` branch runs — that is Hoare's dispatch made visible.

---

## If something goes wrong

| Symptom | Fix |
|---|---|
| `gcc: command not found` | Add `G:\mingw64\bin` to your PATH, or use the full path above |
| `error: unknown type name 'constexpr'` | Your GCC is too old; you need GCC 13 or later with `-std=c2x` |
| Any other error | The `-Werror` flag turned a warning into an error; read the message — the line number is exact |
