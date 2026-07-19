// strassen_bench.c - Benchmark naive vs Strassen
// Compile: gcc-15 -std=c23 -O3 -o bench strassen_bench.c
// Run: ./bench

#define UBENCH_STATIC
#include "ubench.h"

/*
 * gcc_defer.h provides a macro polyfill for the `defer` keyword using GCC's
 * nested-function + cleanup-attribute extension (technique by Jens Gustedt).
 *
 * GCC 16+ is expected to support `defer` natively via the `-fdefer-ts` flag
 * (ISO/DIS TS 25755), which causes the compiler to define __STDC_DEFER_TS25755__.
 * In that case the polyfill must be omitted — including it would redefine
 * `defer` and break compilation.  GCC 15.x does not yet ship this flag.
 *
 * Guarding on a GCC version number would be wrong: the flag is opt-in, so
 * even GCC 16 without `-fdefer-ts` does not define __STDC_DEFER_TS25755__ and
 * still needs the polyfill.  The macro is the correct, version-independent
 * signal.
 *
 * Note: MSVC/IntelliSense will flag the `auto void F(int*)` inside
 * gcc_defer.h as invalid — those are false positives.  The header is
 * GCC-only and requires GNU extensions (-std=gnu23 or implicit on gcc).
 */
#ifndef __STDC_DEFER_TS25755__
#  include "gcc_defer.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define THRESHOLD 64

// Naive matrix multiplication
void naive_mult(int n, double A[n][n], double B[n][n], double C[n][n]) {
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            C[i][j] = 0.0;
            for (int k = 0; k < n; k++)
                C[i][j] += A[i][k] * B[k][j];
        }
}

// Matrix operations
void add(int n, double A[n][n], double B[n][n], double C[n][n]) {
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            C[i][j] = A[i][j] + B[i][j];
}

void sub(int n, double A[n][n], double B[n][n], double C[n][n]) {
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            C[i][j] = A[i][j] - B[i][j];
}

// Strassen's algorithm
void strassen(int n, double A[n][n], double B[n][n], double C[n][n]) {
    if (n <= THRESHOLD) {
        naive_mult(n, A, B, C);
        return;
    }

    int m = n / 2;

    double (*A11)[m] = malloc(sizeof(double[m][m])); defer { free(A11); };
    double (*A12)[m] = malloc(sizeof(double[m][m])); defer { free(A12); };
    double (*A21)[m] = malloc(sizeof(double[m][m])); defer { free(A21); };
    double (*A22)[m] = malloc(sizeof(double[m][m])); defer { free(A22); };
    double (*B11)[m] = malloc(sizeof(double[m][m])); defer { free(B11); };
    double (*B12)[m] = malloc(sizeof(double[m][m])); defer { free(B12); };
    double (*B21)[m] = malloc(sizeof(double[m][m])); defer { free(B21); };
    double (*B22)[m] = malloc(sizeof(double[m][m])); defer { free(B22); };
    double (*M1)[m] = malloc(sizeof(double[m][m]));  defer { free(M1); };
    double (*M2)[m] = malloc(sizeof(double[m][m]));  defer { free(M2); };
    double (*M3)[m] = malloc(sizeof(double[m][m]));  defer { free(M3); };
    double (*M4)[m] = malloc(sizeof(double[m][m]));  defer { free(M4); };
    double (*M5)[m] = malloc(sizeof(double[m][m]));  defer { free(M5); };
    double (*M6)[m] = malloc(sizeof(double[m][m]));  defer { free(M6); };
    double (*M7)[m] = malloc(sizeof(double[m][m]));  defer { free(M7); };
    double (*T1)[m] = malloc(sizeof(double[m][m]));  defer { free(T1); };
    double (*T2)[m] = malloc(sizeof(double[m][m]));  defer { free(T2); };

    // Split
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < m; j++) {
            A11[i][j] = A[i][j];
            A12[i][j] = A[i][j + m];
            A21[i][j] = A[i + m][j];
            A22[i][j] = A[i + m][j + m];
            B11[i][j] = B[i][j];
            B12[i][j] = B[i][j + m];
            B21[i][j] = B[i + m][j];
            B22[i][j] = B[i + m][j + m];
        }
    }

    // Compute M1-M7
    add(m, A11, A22, T1); add(m, B11, B22, T2); strassen(m, T1, T2, M1);
    add(m, A21, A22, T1); strassen(m, T1, B11, M2);
    sub(m, B12, B22, T2); strassen(m, A11, T2, M3);
    sub(m, B21, B11, T2); strassen(m, A22, T2, M4);
    add(m, A11, A12, T1); strassen(m, T1, B22, M5);
    sub(m, A21, A11, T1); add(m, B11, B12, T2); strassen(m, T1, T2, M6);
    sub(m, A12, A22, T1); add(m, B21, B22, T2); strassen(m, T1, T2, M7);

    // Combine
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < m; j++) {
            C[i][j] = M1[i][j] + M4[i][j] - M5[i][j] + M7[i][j];
            C[i][j + m] = M3[i][j] + M5[i][j];
            C[i + m][j] = M2[i][j] + M4[i][j];
            C[i + m][j + m] = M1[i][j] - M2[i][j] + M3[i][j] + M6[i][j];
        }
    }
}

// Benchmarks for 128x128
UBENCH_EX(matrix_128, naive) {
    int n = 128;
    double (*A)[n] = malloc(sizeof(double[n][n]));
    double (*B)[n] = malloc(sizeof(double[n][n]));
    double (*C)[n] = malloc(sizeof(double[n][n]));

    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            A[i][j] = i + j;
            B[i][j] = i - j;
        }

    UBENCH_DO_BENCHMARK() {
        naive_mult(n, A, B, C);
    }

    free(A); free(B); free(C);
}

UBENCH_EX(matrix_128, strassen) {
    int n = 128;
    double (*A)[n] = malloc(sizeof(double[n][n])); defer { free(A); };
    double (*B)[n] = malloc(sizeof(double[n][n])); defer { free(B); };
    double (*C)[n] = malloc(sizeof(double[n][n])); defer { free(C); };

    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            A[i][j] = i + j;
            B[i][j] = i - j;
        }

    UBENCH_DO_BENCHMARK() {
        strassen(n, A, B, C);
    }

    // free(A); free(B); free(C);
}

// Benchmarks for 256x256
UBENCH_EX(matrix_256, naive) {
    int n = 256;
    double (*A)[n] = malloc(sizeof(double[n][n])); defer { free(A); };
    double (*B)[n] = malloc(sizeof(double[n][n])); defer { free(B); };
    double (*C)[n] = malloc(sizeof(double[n][n])); defer { free(C); };
    
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            A[i][j] = i + j;
            B[i][j] = i - j;
        }
    
    UBENCH_DO_BENCHMARK() {
        naive_mult(n, A, B, C);
    }
    
    // free(A); free(B); free(C);
}

UBENCH_EX(matrix_256, strassen) {
    int n = 256;
    double (*A)[n] = malloc(sizeof(double[n][n])); defer { free(A); };
    double (*B)[n] = malloc(sizeof(double[n][n])); defer { free(B); };
    double (*C)[n] = malloc(sizeof(double[n][n])); defer { free(C); };
    
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            A[i][j] = i + j;
            B[i][j] = i - j;
        }
    
    UBENCH_DO_BENCHMARK() {
        strassen(n, A, B, C);
    }
    
    // free(A); free(B); free(C);
}

// Benchmarks for 512x512
UBENCH_EX(matrix_512, naive) {
    int n = 512;
    double (*A)[n] = malloc(sizeof(double[n][n])); defer { free(A); };
    double (*B)[n] = malloc(sizeof(double[n][n])); defer { free(B); };
    double (*C)[n] = malloc(sizeof(double[n][n])); defer { free(C); };

    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            A[i][j] = i + j;
            B[i][j] = i - j;
        }

    UBENCH_DO_BENCHMARK() {
        naive_mult(n, A, B, C);
    }

    // free(A); free(B); free(C);
}

UBENCH_EX(matrix_512, strassen) {
    int n = 512;
    double (*A)[n] = malloc(sizeof(double[n][n])); defer { free(A); };
    double (*B)[n] = malloc(sizeof(double[n][n])); defer { free(B); };
    double (*C)[n] = malloc(sizeof(double[n][n])); defer { free(C); };

    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            A[i][j] = i + j;
            B[i][j] = i - j;
        }

    UBENCH_DO_BENCHMARK() {
        strassen(n, A, B, C);
    }

    // free(A); free(B); free(C);
}

// instead of UBENCH_MAIN(); we go by foot

  UBENCH_STATE();                         
  int main(int argc, const char *const argv[]) { 
    return ubench_main(argc, argv);              
  }