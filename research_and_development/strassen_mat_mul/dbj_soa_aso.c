#include <dbj_ubenchtest.h>
#include <dbj_macros.h>
#include <dbj_clintro.h>

#define DBJ_STRASSEN_MATMUL_IMPLEMENTATION
#include "dbj_strassen_matmul.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

UBENCH_STATE();

typedef enum { A, B, C } GridID;
#define GRIDNUM 3
// smallest power of two above DBJ_STRASSEN_THRESHOLD (64, see
// dbj_strassen_matmul.h) so strassen() actually recurses once instead
// of falling straight through to its own base case -- still small
// enough (384 KB per grid struct) to stay on the stack, matching this
// file's stack-only design (unlike soa_aso_comparator.c, which moved
// to heap storage at similar sizes).
#define GRID_SIDE_LEN 128

typedef struct {
    double rows[GRIDNUM][GRID_SIDE_LEN][GRID_SIDE_LEN];
} SoAGrids;

//-------------------------------------------------------------------------------
typedef struct {
    double rows[GRID_SIDE_LEN][GRID_SIDE_LEN];
} AoSGrid;


//-------------------------------------------------------------------------------
static void grid_fill(double M[GRID_SIDE_LEN][GRID_SIDE_LEN]) {
    DBJ_LOOP_AS(i, GRID_SIDE_LEN) {
        DBJ_LOOP_AS(j, GRID_SIDE_LEN) { M[i][j] = 1 + rand() % 10; }
    }
}

static void grid_print(double M[GRID_SIDE_LEN][GRID_SIDE_LEN]) {
    DBJ_LOOP_AS(i, GRID_SIDE_LEN) {
        DBJ_LOOP_AS(j, GRID_SIDE_LEN) { printf(" %.1f", M[i][j]); }
        printf("\n");
    }
}

//-------------------------------------------------------------------------------
static void soa_grid_run ( void )
{
    /* SoA: grid slice g.rows[id] is itself a contiguous GRID_SIDE_LEN x GRID_SIDE_LEN block */
    SoAGrids g = {0};
    grid_fill(g.rows[A]);
    grid_fill(g.rows[B]);
    strassen(GRID_SIDE_LEN, g.rows[A], g.rows[B], g.rows[C]);
    // printf("SoA C:\n"); grid_print(g.rows[C]);
}

static void aos_grid_run ( void )
{
    /* AoS: aos_grids[id].rows is likewise a contiguous GRID_SIDE_LEN x GRID_SIDE_LEN block */
    AoSGrid aos_grids[GRIDNUM] = {};
    grid_fill(aos_grids[A].rows);
    grid_fill(aos_grids[B].rows);
    strassen(GRID_SIDE_LEN, aos_grids[A].rows, aos_grids[B].rows, aos_grids[C].rows);
    // printf("AoS C:\n"); grid_print(aos_grids[C].rows);
}

// Both bodies fill their grids and then multiply, on purpose: the
// filling is part of what the layouts are being compared on. ubench
// decides how many times to run each and how many samples it needs.
UBENCH(grids_128, aos)
{
    aos_grid_run();
}

UBENCH(grids_128, soa)
{
    soa_grid_run();
}
//-------------------------------------------------------------------------------
int main(const int argc, const char *const argv[static argc + 1]) {
    dbj_clintro("dbj_soa_aso", "0.2.0");
    srand((unsigned)time(NULL));
    return ubench_main(argc, argv);
}