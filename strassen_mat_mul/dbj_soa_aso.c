#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef enum { A, B, C } GridID;
#define GRIDNUM 3
#define N 3

typedef struct {
    double rows[GRIDNUM][N][N];
} Grids;

typedef struct {
    double rows[N][N];
} AoSGrid;

static AoSGrid aos_grids[GRIDNUM] = {};

static void base_mult(int n, double A[n][n], double B[n][n], double C[n][n]) {
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            double sum = 0;
            for (int k = 0; k < n; k++)
                sum += A[i][k] * B[k][j];
            C[i][j] = sum;
        }
}

static void fill(double M[N][N]) {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            M[i][j] = 1 + rand() % 10;
}

static void print(double M[N][N]) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++)
            printf(" %.1f", M[i][j]);
        printf("\n");
    }
}

int main(void) {
    srand((unsigned)time(NULL));

    /* SoA: grid slice g.rows[id] is itself a contiguous N x N block */
    Grids g = {0};
    fill(g.rows[A]);
    fill(g.rows[B]);
    base_mult(N, g.rows[A], g.rows[B], g.rows[C]);
    printf("SoA C:\n"); print(g.rows[C]);

    /* AoS: aos_grids[id].rows is likewise a contiguous N x N block */
    fill(aos_grids[A].rows);
    fill(aos_grids[B].rows);
    base_mult(N, aos_grids[A].rows, aos_grids[B].rows, aos_grids[C].rows);
    printf("AoS C:\n"); print(aos_grids[C].rows);

    return 0;
}