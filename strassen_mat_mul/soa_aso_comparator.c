// soa_aso_comparator.c
//
// Build: make      Run: ../builds/soa_aso_comparator.exe
/*
  2026JUL19     DBJ     Added
  2026JUL20     DBJ     Runtime-sized grids (64..1024), naive vs strassen,
                         SoA vs AoS storage layout. Dropped the fixed-N
                         stack variant: at N=1024 three matrices no longer
                         fit the stack, so all storage is heap now.
  2026AUG31     DBJ     Moved from dbj_nanobench to third_party/dbj_ubenchtest.

  SoA vs AoS storage for a small fixed set of square Grids (aka
  Matrices), multiplied both the naive O(n^3) way and via strassen()
  from dbj_strassen_matmul.h. Strassen's advantage over naive grows
  with grid size, so this is benchmarked across N = 64, 128, 256, 512,
  1024 — naive_mult() is the same baseline used in
  strassen_bench_comparator.c.

  GRIDNUM=3 matrices per layout: grid 0 * grid 1 -> grid 2.

    +-----------------------------+   +-----------------------------+
    |   SoA_Grids (heap)          |   |   AoS_Grid[GRIDNUM] (heap)   |
    |  m[GRIDNUM][N][N]           |   |  one malloc per grid          |
    |  one contiguous malloc      |   |  N x N doubles each           |
    +-----------------------------+   +-----------------------------+

  For each N, both naive_mult() and strassen() are benchmarked against
  both layouts: 4 benchmarks per size. Allocating and filling both
  layouts is fixture setup, so it is not timed.

  Twenty benchmarks, and the naive ones at 1024 are minutes each. Run a
  subset with ubench's filter:

      soa_aso_comparator.exe --filter=grids_64.*

  Benchmarks come out in the linker's order, not this file's; the names
  carry size, algorithm and layout, so a shuffled list still reads.
*/

#include <dbj_ubenchtest.h>

#define DBJ_STRASSEN_MATMUL_IMPLEMENTATION
#include "dbj_strassen_matmul.h"

#include <stdlib.h>

UBENCH_STATE();

#define GRIDNUM 3

// Naive matrix multiplication — the benchmark baseline, same as
// strassen_bench_comparator.c's naive_mult().
static void naive_mult(int n, double A[n][n], double B[n][n], double C[n][n])
{
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
        {
            C[i][j] = 0.0;
            for (int k = 0; k < n; k++)
                C[i][j] += A[i][k] * B[k][j];
        }
}

/* Generates, for DBJ_GRID_BENCHMARKS(64):

       grids_64.naive_soa      grids_64.strassen_soa
       grids_64.naive_aos      grids_64.strassen_aos

   The SoA fixture members point into one allocation; the AoS ones are
   three separate allocations. That is the whole difference under test.
   The size is a literal, so the row types are fixed array types and no
   VLA arithmetic enters the timed body. */
#define DBJ_GRID_BENCHMARKS(N_)                                      \
    struct grids_##N_                                                \
    {                                                                \
        double *soa_block;                                           \
        double(*soa[GRIDNUM])[N_];                                   \
        double(*aos[GRIDNUM])[N_];                                   \
    };                                                               \
                                                                     \
    UBENCH_F_SETUP(grids_##N_)                                       \
    {                                                                \
        ubench_fixture->soa_block =                                  \
            malloc(sizeof(double) * GRIDNUM * N_ * N_);              \
        for (int grid = 0; grid < GRIDNUM; grid++)                   \
        {                                                            \
            ubench_fixture->soa[grid] = (double(*)[N_])(             \
                ubench_fixture->soa_block + (size_t)grid * N_ * N_); \
            ubench_fixture->aos[grid] =                              \
                malloc(sizeof(double[N_][N_]));                       \
        }                                                            \
        for (int row = 0; row < N_; row++)                           \
        {                                                            \
            for (int col = 0; col < N_; col++)                       \
            {                                                        \
                ubench_fixture->soa[0][row][col] = row + col;        \
                ubench_fixture->soa[1][row][col] = row - col;        \
                ubench_fixture->aos[0][row][col] = row + col;        \
                ubench_fixture->aos[1][row][col] = row - col;        \
            }                                                        \
        }                                                            \
    }                                                                \
                                                                     \
    UBENCH_F_TEARDOWN(grids_##N_)                                    \
    {                                                                \
        free(ubench_fixture->soa_block);                             \
        for (int grid = 0; grid < GRIDNUM; grid++)                   \
        {                                                            \
            free(ubench_fixture->aos[grid]);                         \
        }                                                            \
    }                                                                \
                                                                     \
    UBENCH_F(grids_##N_, naive_soa)                                  \
    {                                                                \
        naive_mult(N_, ubench_fixture->soa[0], ubench_fixture->soa[1], \
                   ubench_fixture->soa[2]);                          \
    }                                                                \
                                                                     \
    UBENCH_F(grids_##N_, naive_aos)                                  \
    {                                                                \
        naive_mult(N_, ubench_fixture->aos[0], ubench_fixture->aos[1], \
                   ubench_fixture->aos[2]);                          \
    }                                                                \
                                                                     \
    UBENCH_F(grids_##N_, strassen_soa)                               \
    {                                                                \
        strassen(N_, ubench_fixture->soa[0], ubench_fixture->soa[1], \
                 ubench_fixture->soa[2]);                            \
    }                                                                \
                                                                     \
    UBENCH_F(grids_##N_, strassen_aos)                               \
    {                                                                \
        strassen(N_, ubench_fixture->aos[0], ubench_fixture->aos[1], \
                 ubench_fixture->aos[2]);                            \
    }

DBJ_GRID_BENCHMARKS(64)
DBJ_GRID_BENCHMARKS(128)
DBJ_GRID_BENCHMARKS(256)
DBJ_GRID_BENCHMARKS(512)
DBJ_GRID_BENCHMARKS(1024)

int main(const int argc, const char *const argv[static argc + 1])
{
    return ubench_main(argc, argv);
}
