// soa_aso_comparator.c
//
// Compile: make.cmd soa_aso_comparator.exe
// Run:     soa_aso_comparator.exe
/*
  2026JUL19     DBJ     Added

  This is the SoA concept I preffer. I also oreffer using stack vs heap. 
  Example is abot multiplyng Grids (aka Matrices) using the strassen algorithm we have here (dbj_strassen_matmul.h)

  SoA presentation of the Grids

typedef enum { A, B, C } GridID;

#define GRIDNUM 3
#define RowSIZE 3
#define ColSIZE 3

typedef struct {
    int rows[GRIDNUM][RowSIZE];
    int cols[GRIDNUM][ColSIZE];
} Grids;


*/

#include "../dbj_nanobench/dbj_nanobench.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef __STDC_DEFER_TS25755__
#  include <dbj_defer.h>
#endif

int main(int argc, const char *const argv[static argc + 1]) {
    (void)argc; (void)argv;
    dbj_nb_bench_all();
    return 0;
}
