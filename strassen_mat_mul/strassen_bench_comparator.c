// strassen_bench.c - Benchmark naive vs Strassen
// Compile: gcc-15 -std=c23 -O3 -o bench strassen_bench.c
// Run: ./bench

#define UBENCH_STATIC
#include "ubench.h"

// DBJ 2026JUL18 compare UBENCH and DBJ_NANOBENCH to gauge the measurements validity
#include "../../dbj_nanobench/dbj_nanobench.h"

#define DBJ_STRASSEN_MATMUL_IMPLEMENTATION
#include "dbj_strassen_matmul.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Naive matrix multiplication — the benchmark baseline. Not part of
// dbj_strassen_matmul.h: that header's own base case is
// dbj_strassen_base_mult(), private to strassen()'s recursion.
static void naive_mult(int n, double A[n][n], double B[n][n], double C[n][n]) {
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            C[i][j] = 0.0;
            for (int k = 0; k < n; k++)
                C[i][j] += A[i][k] * B[k][j];
        }
}

// Benchmarks for matrix_SIZE.{naive,strassen} — UBENCH_EX registers each
// benchmark by token-pasting SET/NAME into a file-scope function name, so
// the (size x algorithm) grid can't be driven by a runtime loop the way
// dbj_nb_bench_matrix() below is; this macro just removes the boilerplate
// each UBENCH_EX body repeated for every size.
#define STRASSEN_UBENCH_MATRIX(SIZE, NAME, ALGO_FN)               \
    UBENCH_EX(matrix_##SIZE, NAME) {                              \
        int n = SIZE;                                             \
        double (*A)[n] = malloc(sizeof(double[n][n])); defer { free(A); }; \
        double (*B)[n] = malloc(sizeof(double[n][n])); defer { free(B); }; \
        double (*C)[n] = malloc(sizeof(double[n][n])); defer { free(C); }; \
        for (int i = 0; i < n; i++)                               \
            for (int j = 0; j < n; j++) {                         \
                A[i][j] = i + j;                                  \
                B[i][j] = i - j;                                  \
            }                                                     \
        UBENCH_DO_BENCHMARK() {                                   \
            ALGO_FN(n, A, B, C);                                  \
        }                                                         \
    }

STRASSEN_UBENCH_MATRIX(128, naive, naive_mult)
STRASSEN_UBENCH_MATRIX(128, strassen, strassen)
STRASSEN_UBENCH_MATRIX(256, naive, naive_mult)
STRASSEN_UBENCH_MATRIX(256, strassen, strassen)
STRASSEN_UBENCH_MATRIX(512, naive, naive_mult)
STRASSEN_UBENCH_MATRIX(512, strassen, strassen)
STRASSEN_UBENCH_MATRIX(1024, naive, naive_mult)
STRASSEN_UBENCH_MATRIX(1024, strassen, strassen)

#undef STRASSEN_UBENCH_MATRIX

// --- DBJ_NANOBENCH cross-check ---
// Same workloads as the UBENCH_EX blocks above, run through dbj_nanobench
// instead, so the two harnesses' numbers can be compared side by side to
// gauge measurement validity (see comparator include above).

static void dbj_nb_bench_matrix(int n) {
    double (*A)[n] = malloc(sizeof(double[n][n])); defer { free(A); };
    double (*B)[n] = malloc(sizeof(double[n][n])); defer { free(B); };
    double (*C)[n] = malloc(sizeof(double[n][n])); defer { free(C); };

    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            A[i][j] = i + j;
            B[i][j] = i - j;
        }

    char naive_name[64];
    char strassen_name[64];
    snprintf(naive_name, sizeof(naive_name), "matrix_%d naive (nanobench)", n);
    snprintf(strassen_name, sizeof(strassen_name), "matrix_%d strassen (nanobench)", n);

    DBJ_BENCH(naive_name, double, 3, 10, {
        naive_mult(n, A, B, C);
        DBJ_NB_val = C[0][0];
    });

    DBJ_BENCH(strassen_name, double, 3, 10, {
        strassen(n, A, B, C);
        DBJ_NB_val = C[0][0];
    });
}

static void dbj_nb_bench_all(void) {
    dbj_nb_bench_matrix(128);
    dbj_nb_bench_matrix(256);
    dbj_nb_bench_matrix(512);
    dbj_nb_bench_matrix(1024);
}

// instead of UBENCH_MAIN(); we go by foot

UBENCH_STATE();
int main(int argc, const char *const argv[static argc + 1]) {
    int rc_ = ubench_main(argc, argv);
    dbj_nb_bench_all();
    return rc_;
}