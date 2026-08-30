// strassen_bench_comparator.c - naive vs Strassen, on
// third_party/dbj_ubenchtest.
//
// Same question as strassen.c, but against this file's own naive_mult()
// rather than the header's internal base case, and one size larger.
//
// Build: make      Run: ../builds/strassen_bench_comparator.exe
//
// matrix_1024.naive is minutes of work on its own. Run a subset with
// ubench's filter:  strassen_bench_comparator.exe --filter=matrix_128.*

#include <dbj_ubenchtest.h>

#define DBJ_STRASSEN_MATMUL_IMPLEMENTATION
#include "dbj_strassen_matmul.h"

#include <stdlib.h>

UBENCH_STATE();

// Naive matrix multiplication — the benchmark baseline. Not part of
// dbj_strassen_matmul.h: that header's own base case is
// dbj_strassen_base_mult(), private to strassen()'s recursion.
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

/* Allocating and filling the matrices is fixture setup, so it is not
   timed. The body is one multiply -- ubench does the looping. */
#define DBJ_MATRIX_BENCHMARKS(N_)                                     \
    struct matrix_##N_                                                \
    {                                                                 \
        double(*A)[N_];                                               \
        double(*B)[N_];                                               \
        double(*C)[N_];                                               \
    };                                                                \
                                                                      \
    UBENCH_F_SETUP(matrix_##N_)                                       \
    {                                                                 \
        ubench_fixture->A = malloc(sizeof(double[N_][N_]));            \
        ubench_fixture->B = malloc(sizeof(double[N_][N_]));            \
        ubench_fixture->C = malloc(sizeof(double[N_][N_]));            \
        for (int row = 0; row < N_; row++)                            \
        {                                                             \
            for (int col = 0; col < N_; col++)                        \
            {                                                         \
                ubench_fixture->A[row][col] = row + col;              \
                ubench_fixture->B[row][col] = row - col;              \
            }                                                         \
        }                                                             \
    }                                                                 \
                                                                      \
    UBENCH_F_TEARDOWN(matrix_##N_)                                    \
    {                                                                 \
        free(ubench_fixture->A);                                      \
        free(ubench_fixture->B);                                      \
        free(ubench_fixture->C);                                      \
    }                                                                 \
                                                                      \
    UBENCH_F(matrix_##N_, naive)                                      \
    {                                                                 \
        naive_mult(N_, ubench_fixture->A, ubench_fixture->B,          \
                   ubench_fixture->C);                                \
    }                                                                 \
                                                                      \
    UBENCH_F(matrix_##N_, strassen)                                   \
    {                                                                 \
        strassen(N_, ubench_fixture->A, ubench_fixture->B,            \
                 ubench_fixture->C);                                  \
    }

DBJ_MATRIX_BENCHMARKS(128)
DBJ_MATRIX_BENCHMARKS(256)
DBJ_MATRIX_BENCHMARKS(512)
DBJ_MATRIX_BENCHMARKS(1024)

int main(const int argc, const char *const argv[static argc + 1])
{
    return ubench_main(argc, argv);
}
