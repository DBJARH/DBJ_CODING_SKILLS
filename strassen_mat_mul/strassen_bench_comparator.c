// strassen_bench.c - Benchmark naive vs Strassen
// Compile: gcc-15 -std=c23 -O3 -o bench strassen_bench.c
// Run: ./bench

#include "../dbj_nanobench/dbj_nanobench.h"

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

    DBJ_BENCH_N(naive_name, double, 3, 10, {
        naive_mult(n, A, B, C);
        DBJ_NB_val = C[0][0];
    });

    DBJ_BENCH_N(strassen_name, double, 3, 10, {
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

int main(int argc, const char *const argv[static argc + 1]) {
    (void)argc; (void)argv;
    dbj_nb_bench_all();
    return 0;
}