#pragma once
/*
    2026JUL20       (c) dbj@dbj.org

    Standard CLI intro screen for dbj CLI apps, Windows and Linux alike:
    one call near the top of main() prints a big ASCII-art "DBJ" banner
    followed by the app name, version, host OS and build timestamp.

        #include <dbj_clintro.h>
        int main(void) {
            dbj_clintro("dbj_email_crud", "1.0.0");
            ...
        }
*/
#include <dbj_required_compile_time.h>
#include <dbj_macros.h>

#include <stdio.h>

#if defined(_WIN32)
#define DBJ_CLINTRO_OS_NAME "Windows"
#else
#define DBJ_CLINTRO_OS_NAME "Linux"
#endif

#define DBJ_CLINTRO_GLYPH_ROWS 7
#define DBJ_CLINTRO_GLYPH_COLS 6
#define DBJ_CLINTRO_GLYPH_COUNT 3 /* D, B, J */

/* 7x6 block-letter bitmap for "DBJ", one row of 6 chars per glyph row --
   '#' is ink, ' ' is blank. Order: D, B, J. */
static const char DBJ_CLINTRO_GLYPHS
    [DBJ_CLINTRO_GLYPH_COUNT][DBJ_CLINTRO_GLYPH_ROWS][DBJ_CLINTRO_GLYPH_COLS + 1] = {
    { /* D */
        "##### ",
        "##  ##",
        "##  ##",
        "##  ##",
        "##  ##",
        "##  ##",
        "##### ",
    },
    { /* B */
        "##### ",
        "##  ##",
        "##  ##",
        "##### ",
        "##  ##",
        "##  ##",
        "##### ",
    },
    { /* J */
        "   ###",
        "    ##",
        "    ##",
        "    ##",
        "##  ##",
        "##  ##",
        " #### ",
    },
};

static inline void dbj_clintro_banner(void) {
    DBJ_LOOP_AS(row, DBJ_CLINTRO_GLYPH_ROWS) {
        DBJ_LOOP_AS(glyph, DBJ_CLINTRO_GLYPH_COUNT) {
            printf("%s  ", DBJ_CLINTRO_GLYPHS[glyph][row]);
        }
        printf("\n");
    }
}

static inline void dbj_clintro(const char app_name[static 1], const char version[static 1]) {
    dbj_clintro_banner();
    printf("\n");
    printf("%s  version %s\n", app_name, version);
    printf("OS: %s   built: %s %s\n", DBJ_CLINTRO_OS_NAME, __DATE__, __TIME__);
    printf("\n");
}
