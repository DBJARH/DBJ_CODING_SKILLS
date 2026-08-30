// strassen.c - Benchmark naive vs Strassen, on third_party/dbj_ubenchtest.
// Build: make      Run: ../builds/bench.exe
//
// One fixture per matrix size. Allocating and filling the matrices is
// setup, so it is not timed; the body is a single multiply, which is
// what ubench wants -- it does the looping and the sampling itself.
//
// Benchmarks print in the linker's order, not this file's. The names
// carry the size, so a shuffled list still reads.

#include <dbj_ubenchtest.h>

#define DBJ_STRASSEN_MATMUL_IMPLEMENTATION
#include "dbj_strassen_matmul.h"

#include <stdlib.h>

UBENCH_STATE();

/* Generates, for DBJ_MATRIX_BENCHMARKS(128):

       struct matrix_128        the three matrices
       matrix_128.naive         one dbj_strassen_base_mult, timed
       matrix_128.strassen      one strassen, timed

   The size is a literal, so the matrix types are ordinary fixed array
   types and no VLA arithmetic enters the timed body. */
#define DBJ_MATRIX_BENCHMARKS(N_)                                    \
    struct matrix_##N_                                               \
    {                                                                \
        double(*A)[N_];                                              \
        double(*B)[N_];                                              \
        double(*C)[N_];                                              \
    };                                                               \
                                                                     \
    UBENCH_F_SETUP(matrix_##N_)                                      \
    {                                                                \
        ubench_fixture->A = malloc(sizeof(double[N_][N_]));           \
        ubench_fixture->B = malloc(sizeof(double[N_][N_]));           \
        ubench_fixture->C = malloc(sizeof(double[N_][N_]));           \
        for (int row = 0; row < N_; row++)                           \
        {                                                            \
            for (int col = 0; col < N_; col++)                       \
            {                                                        \
                ubench_fixture->A[row][col] = row + col;             \
                ubench_fixture->B[row][col] = row - col;             \
            }                                                        \
        }                                                            \
    }                                                                \
                                                                     \
    UBENCH_F_TEARDOWN(matrix_##N_)                                   \
    {                                                                \
        free(ubench_fixture->A);                                     \
        free(ubench_fixture->B);                                     \
        free(ubench_fixture->C);                                     \
    }                                                                \
                                                                     \
    UBENCH_F(matrix_##N_, naive)                                     \
    {                                                                \
        dbj_strassen_base_mult(N_, ubench_fixture->A,                \
                               ubench_fixture->B, ubench_fixture->C); \
    }                                                                \
                                                                     \
    UBENCH_F(matrix_##N_, strassen)                                  \
    {                                                                \
        strassen(N_, ubench_fixture->A, ubench_fixture->B,           \
                 ubench_fixture->C);                                 \
    }

DBJ_MATRIX_BENCHMARKS(128)
DBJ_MATRIX_BENCHMARKS(256)
DBJ_MATRIX_BENCHMARKS(512)

int main(const int argc, const char *const argv[static argc + 1])
{
    return ubench_main(argc, argv);
}
