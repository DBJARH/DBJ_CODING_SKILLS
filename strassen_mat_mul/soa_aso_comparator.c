// soa_aso_comparator.c
//
// Compile: make.cmd soa_aso_comparator.exe
// Run:     soa_aso_comparator.exe
/*
  2026JUL19     DBJ     Added
  2026JUL20     DBJ     Runtime-sized grids (64..1024), naive vs strassen,
                         SoA vs AoS storage layout. Dropped the fixed-N
                         stack variant: at N=1024 three matrices no longer
                         fit the stack, so all storage is heap now.

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
  both layouts: 4 benchmarks per size.
*/

#include "../dbj_nanobench/dbj_nanobench.h"

#define DBJ_STRASSEN_MATMUL_IMPLEMENTATION
#include "dbj_strassen_matmul.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef __STDC_DEFER_TS25755__
#  include <dbj_defer.h>
#endif

#define GRIDNUM 3

// Struct-of-Arrays: all GRIDNUM n x n matrices in one contiguous block.
typedef struct {
    double *m; // [GRIDNUM][n][n], flattened
} SoA_Grids;

// Array-of-Structs: one separately allocated n x n matrix per grid.
typedef struct {
    double *m[GRIDNUM]; // each [n][n], flattened
} AoS_Grids;

static void soa_grids_alloc(SoA_Grids *g, int n) {
    g->m = malloc(sizeof(double) * GRIDNUM * n * n);
}

static void soa_grids_free(SoA_Grids *g) {
    free(g->m);
}

static void aos_grids_alloc(AoS_Grids *g, int n) {
    for (int k = 0; k < GRIDNUM; k++)
        g->m[k] = malloc(sizeof(double) * n * n);
}

static void aos_grids_free(AoS_Grids *g) {
    for (int k = 0; k < GRIDNUM; k++)
        free(g->m[k]);
}

static void soa_grids_fill(SoA_Grids *g, int n) {
    double (*grid0)[n] = (double (*)[n])(g->m + 0 * n * n);
    double (*grid1)[n] = (double (*)[n])(g->m + 1 * n * n);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            grid0[i][j] = i + j;
            grid1[i][j] = i - j;
        }
}

static void aos_grids_fill(AoS_Grids *g, int n) {
    double (*grid0)[n] = (double (*)[n])g->m[0];
    double (*grid1)[n] = (double (*)[n])g->m[1];
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            grid0[i][j] = i + j;
            grid1[i][j] = i - j;
        }
}

// Naive matrix multiplication — the benchmark baseline, same as
// strassen_bench_comparator.c's naive_mult().
static void naive_mult(int n, double A[n][n], double B[n][n], double C[n][n]) {
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            C[i][j] = 0.0;
            for (int k = 0; k < n; k++)
                C[i][j] += A[i][k] * B[k][j];
        }
}

static void dbj_nb_bench_grids(int n) {
    SoA_Grids soa; soa_grids_alloc(&soa, n); defer { soa_grids_free(&soa); };
    soa_grids_fill(&soa, n);
    double (*soa0)[n] = (double (*)[n])(soa.m + 0 * n * n);
    double (*soa1)[n] = (double (*)[n])(soa.m + 1 * n * n);
    double (*soa2)[n] = (double (*)[n])(soa.m + 2 * n * n);

    AoS_Grids aos; aos_grids_alloc(&aos, n); defer { aos_grids_free(&aos); };
    aos_grids_fill(&aos, n);
    double (*aos0)[n] = (double (*)[n])aos.m[0];
    double (*aos1)[n] = (double (*)[n])aos.m[1];
    double (*aos2)[n] = (double (*)[n])aos.m[2];

    char naive_soa_name[64];
    char naive_aos_name[64];
    char strassen_soa_name[64];
    char strassen_aos_name[64];
    snprintf(naive_soa_name, sizeof(naive_soa_name), "grids_%d naive SoA (nanobench)", n);
    snprintf(naive_aos_name, sizeof(naive_aos_name), "grids_%d naive AoS (nanobench)", n);
    snprintf(strassen_soa_name, sizeof(strassen_soa_name), "grids_%d strassen SoA (nanobench)", n);
    snprintf(strassen_aos_name, sizeof(strassen_aos_name), "grids_%d strassen AoS (nanobench)", n);

    DBJ_BENCH(naive_soa_name, double, 3, 10, {
        naive_mult(n, soa0, soa1, soa2);
        DBJ_NB_val = soa2[0][0];
    });

    DBJ_BENCH(naive_aos_name, double, 3, 10, {
        naive_mult(n, aos0, aos1, aos2);
        DBJ_NB_val = aos2[0][0];
    });

    DBJ_BENCH(strassen_soa_name, double, 3, 10, {
        strassen(n, soa0, soa1, soa2);
        DBJ_NB_val = soa2[0][0];
    });

    DBJ_BENCH(strassen_aos_name, double, 3, 10, {
        strassen(n, aos0, aos1, aos2);
        DBJ_NB_val = aos2[0][0];
    });
}

static void dbj_nb_bench_all(void) {
    dbj_nb_bench_grids(64);
    dbj_nb_bench_grids(128);
    dbj_nb_bench_grids(256);
    dbj_nb_bench_grids(512);
    dbj_nb_bench_grids(1024);
}

int main(int argc, const char *const argv[static argc + 1]) {
    (void)argc; (void)argv;
    dbj_nb_bench_all();
    return 0;
}
