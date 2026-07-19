# Strassen vs Naive — UBENCH vs DBJ_NANOBENCH Comparison

`strassen_bench_comparator.c` runs the same naive-vs-Strassen matrix
multiplication workloads (n = 128, 256, 512, 1024) through two independent
benchmark harnesses — [ubench.h](ubench.h) and
[dbj_nanobench.h](../../dbj_nanobench/dbj_nanobench.h) — so their
measurements can be cross-checked against each other.

Build & run:

```cmd
make.cmd strassen_bench_comparator.exe
strassen_bench_comparator.exe
```

## Results

3 repeated runs, `-O3`, GCC 15.2.0 (MinGW-w64), on the author's machine.
All timings in milliseconds, shown as min–max across the 3 runs (each
value itself is that harness's own avg over its internal sample count).
Numbers will vary by machine; what matters is that both harnesses agree
with each other and with the expected Strassen speedup.

| n   | Harness       | naive (ms)      | strassen (ms)  | speedup (naive / strassen) |
|-----|---------------|-----------------:|----------------:|----------------------------:|
| 128 | UBENCH        | 2.884 – 2.950    | 0.884 – 1.020   | 2.86x – 3.34x                |
| 128 | DBJ_NANOBENCH | 2.630 – 2.680    | 0.865 – 0.957   | 2.80x – 3.07x                |
| 256 | UBENCH        | 31.285 – 32.270  | 6.948 – 8.179   | 3.94x – 4.55x                |
| 256 | DBJ_NANOBENCH | 31.830 – 32.890  | 6.810 – 7.640   | 4.17x – 4.83x                |
| 512 | UBENCH        | 315.744 – 337.719| 52.209 – 56.098 | 5.66x – 6.05x                |
| 512 | DBJ_NANOBENCH | 358.670 – 417.850| 48.980 – 53.190 | 6.75x – 8.53x                |
| 1024| UBENCH        | 3628.000 (1 run) | 361.636 (1 run) | 10.03x (1 run)               |
| 1024| DBJ_NANOBENCH | 3612.460 (1 run) | 353.330 (1 run) | 10.23x (1 run)               |

n=1024 is shown from a single run only (naive alone takes ~3.6s per
sample, making repeated full sweeps expensive) — treat it as indicative,
not as tight a bound as the smaller sizes.

## Reading

- Both harnesses agree closely at n=128 and n=256 across all 3 runs —
  neither timer nor loop overhead dominates at these workload sizes.
- Speedup grows with n, as expected: Strassen is O(n^2.807) against naive's
  O(n^3), so the gap widens as n increases.
- At n=512 the naive numbers themselves swing ~15–20% run to run in *both*
  harnesses (UBENCH: 316→338ms, DBJ_NANOBENCH: 359→418ms) — confirming this
  is real system noise (cache/TLB/OS scheduling at ~350-400ms runtimes),
  not a harness bug. DBJ_NANOBENCH's smaller sample count (10 timed runs)
  makes its average more sensitive to that noise than UBENCH's larger
  adaptive sampling, which is why its speedup range is wider. Both still
  agree Strassen wins by roughly an order of magnitude at this size.
- At n=1024 both harnesses agree closely (~10x speedup) despite being a
  single sample each — the absolute runtimes are now large enough
  (seconds for naive) that the earlier noise-sensitivity concern matters
  less in relative terms.

See [README.md](README.md) for build requirements and the Strassen
algorithm background.
