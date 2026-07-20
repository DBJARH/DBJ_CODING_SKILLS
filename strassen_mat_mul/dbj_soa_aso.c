#define DBJ_NANOBENCH_IMPLEMENTATION
#include <dbj_nanobench.h>
#include "../toplevel/dbj_macros.h"
#include "../toplevel/dbj_clintro.h"

#define DBJ_STRASSEN_MATMUL_IMPLEMENTATION
#include "dbj_strassen_matmul.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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

static void dbj_nb_bench_all(void) {
    // strassen(GRID_SIDE_LEN, ...) recurses with heap-allocated
    // temporaries per call -- DBJ_MEASURE's defaults (1000 warmup +
    // 100000 timed) are sized for cheap in-place work and would run
    // for a very long time here, so use small explicit counts instead,
    // matching strassen_bench_comparator.c / soa_aso_comparator.c.
    DBJ_MEASURE_N("AOS BENCH", 3, 10, {
        aos_grid_run();
    });

    DBJ_MEASURE_N("SOA BENCH", 3, 10, {
        soa_grid_run();
    });
}
//-------------------------------------------------------------------------------
int main(void) {
    dbj_clintro("dbj_soa_aso", "0.1.0");
    srand((unsigned)time(NULL));
    dbj_nb_bench_all();
    return 42;
}