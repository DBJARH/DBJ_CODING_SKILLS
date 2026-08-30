# [Strassen Matrix Multiplication](#strassen-matrix-multiplication-benchmark) Benchmark

Benchmarks comparing naive O(n^3) matrix multiplication against Strassen's O(n^2.807) algorithm using [dbj_ubenchtest](../third_party/dbj_ubenchtest/dbj_ubenchtest.h).

Four programs, all on ubenchtest: `strassen.c` (naive base case vs strassen), `strassen_bench_comparator.c` (same, against its own naive baseline, up to 1024), `soa_aso_comparator.c` (both algorithms against both storage layouts, 64..1024) and `dbj_soa_aso.c` (the stack-only SoA/AoS pair at 128).

Allocating and filling matrices happens in a fixture, so it is never timed; each benchmark body is one multiply and ubenchtest decides how often to run it. Benchmarks print in the linker's order, not source order — the names carry size, algorithm and layout. The large sizes are minutes of work, so run a subset with the filter:

```cmd
soa_aso_comparator.exe --filter=grids_64.*
```

Measured here, `-O3`, 512x512: naive 316 ms against strassen 50 ms, both within ±2.5%.

## Requirements

- MINGW (GCC 15+) with C23 support
- Tested with: `gcc.exe (MinGW-W64 x86_64-msvcrt-posix-seh, built by Brecht Sanders, r1) 15.2.0`
- `<dbj_defer.h>` (in `../toplevel`, resolved via `-I../toplevel`) contains
  the Gustedt defer macro, that requires GCC. `gcc_defer.h` in this folder
  is deprecated — kept for reference only, not included by any code here.
- there are comments on gcc and defer, in case you are really interested you can read them

> [!Important]
>
> Built on my machine with `G:\mingw64` present. In Makefile that path is defined, change in tune with your local path.
>
> MINGW is essentially a Windows exercise. 

## Build & Run

Use `make.cmd` found here.

```cmd
make
```

or

```cmd
make clean
```

`dbj_str_lib\strassen_mat_mul\dbj_bench_win_run.exe` is left in here. One can run it and see the "thing" in action. Proving the (surprising) superiority of the strassen matmul.

## Appendix: The Strassen Idea

Standard matrix multiplication of two n x n matrices requires n^3 scalar multiplications. Strassen's key insight (1969) is that the product of two 2x2 matrices can be computed with **7 multiplications** instead of the usual 8, at the cost of extra additions. By applying this trick recursively — splitting each n x n matrix into four n/2 x n/2 blocks and combining them with 7 recursive multiplies instead of 8 — the total complexity drops from O(n^3) to O(n^log2(7)) ~ O(n^2.807). The trade-off is more additions and subtractions per level, but since the number of multiplications dominates at scale, the algorithm wins for sufficiently large matrices.

Jump back to [Requirements](#requirements)

---

GCC defer inspired by https://antonz.org/defer-in-c/#final-thoughts

---

(c) 2026 by dbj@dbj.org | MIT license
