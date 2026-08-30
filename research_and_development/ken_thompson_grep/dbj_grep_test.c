/*
    2026JUL30       (c) dbj@dbj.org

    gcc -std=c23 -Wall -Wextra -Wswitch -Werror -I "$DBJ_CORELIB" -o dbj_grep dbj_grep_test.c

    See readme.md for the design this implements, and for what
    ken_thompson_grep.md in this folder actually is (short version: an
    LLM reconstruction with the regex compiler missing, kept unfixed as
    the baseline).

    A C23 port of the regular expression engine from Ken Thompson's
    Version 6 Unix grep (c. 1975), with the opcode stream -- which the
    original encoded as untyped bytes in one flat char expbuf[512] --
    made an explicit tagged union dispatched by an exhaustive switch.
    That engine is dbj_grep.h; this file is only its driver.

    Single header (STB) style: dbj_grep.h is declaration-only
    unless DBJ_GREP_IMPLEMENTATION is defined first, as below.

    Three modes:

        dbj_grep <pattern> [file...]  -- grep, reading file(s) or stdin
        dbj_grep --selftest           -- run the built-in self test
        dbj_grep --help               -- print usage

    With no arguments at all it prints usage and exits 1; the self test
    runs only when asked for by name.

    The self test is the point of the POC; the grep mode is there so the
    thing can be pointed at real input. Deliberately no -v/-c/-n/-b/-l
    flags: this demonstrates the matcher, and the original's flag
    handling is the part of that file with the least to say about tagged
    unions -- see readme.md, "What was deliberately not carried
    over".
*/
#include <stdio.h>
#include <string.h>

#define DBJ_GREP_IMPLEMENTATION
#include "dbj_grep.h"

#include <dbj_defer.h>
#include <dbj_simple_log.h>
#include <dbj_clintro.h>

/* The original's linebuf[CBSIZE] was 512 bytes -- which is exactly what
   <stdio.h> now calls BUFSIZ on this toolchain, the same "one stdio
   buffer's worth" idea under a standard name. So use BUFSIZ rather than
   restating 512: it is the modern spelling of the same decision, and on
   a platform where a stdio buffer is bigger (glibc uses 8192) a longer
   line is simply handled in one read instead of two.

   Either way nothing is truncated -- fgets splits a longer input line
   across reads, exactly as the original did. */
#define GREP_LINE_SIZE BUFSIZ

/* Most files accepted on one command line. The engine's own limits live
   in dbj_grep.h ("LIMITS"); this one is the driver's, because it is the
   driver that walks argv.

   Nothing here would actually break at a higher count -- files are
   opened and closed one at a time, so there is no per-file storage to
   overflow. The limit exists so the refusal is explicit and reported,
   rather than the program quietly grinding through an argv that a shell
   glob expanded to something absurd. */
#define GREP_MAX_FILES 256

/* ── self test ──────────────────────────────────────────────────────
   One table of cases, run in one loop. Each case states the pattern,
   the subject line and whether it should match -- so a case reads as
   the claim it is making, and a failure prints all three. */

typedef struct {
    const char* pattern;
    const char* line;
    bool should_match;
} DbjGrepMatchCase;

static const DbjGrepMatchCase DBJ_GREP_MATCH_CASES[] = {
    /* literals */
    {"abc",         "abc",              true},
    {"abc",         "xxabcxx",          true},
    {"abc",         "ab",               false},
    {"abc",         "",                 false},
    {"",            "anything",         true},

    /* . matches any one character, but not end of line */
    {"a.c",         "abc",              true},
    {"a.c",         "a c",              true},
    {"a.c",         "ac",               false},
    {".",           "",                 false},

    /* ^ anchors at line start */
    {"^abc",        "abc",              true},
    {"^abc",        "xabc",             false},
    {"^",           "anything",         true},

    /* $ anchors at line end, and is a literal anywhere else */
    {"abc$",        "abc",              true},
    {"abc$",        "abcx",             false},
    {"a$b",         "a$b",              true},

    /* ^...$ is a whole-line match */
    {"^abc$",       "abc",              true},
    {"^abc$",       "abcd",             false},
    {"^$",          "",                 true},
    {"^$",          "x",                false},

    /* star, including the zero-repetition case */
    {"ab*c",        "ac",               true},
    {"ab*c",        "abc",              true},
    {"ab*c",        "abbbbc",           true},
    {"ab*c",        "abbbbd",           false},
    {"^a*$",        "",                 true},
    {"^a*$",        "aaaa",             true},
    {"^a*$",        "aaab",             false},

    /* .* -- the greedy case that must give characters back */
    {"^a.*c$",      "abbbc",            true},
    {"^a.*z$",      "abbbc",            false},
    {"a.*b.*c",     "axxbyyc",          true},

    /* character classes */
    {"[abc]",       "xbx",              true},
    {"[abc]",       "xyz",              false},
    {"^[0-9][0-9]$", "42",              true},
    {"^[0-9][0-9]$", "4a",              false},
    {"[a-z]*",      "",                 true},
    {"^[a-z]*$",    "hello",            true},
    {"^[a-z]*$",    "hell0",            false},

    /* negated classes */
    {"^[^0-9]$",    "a",                true},
    {"^[^0-9]$",    "7",                false},
    {"^[^a-z]*$",   "123",              true},

    /* a ] in first position is a literal ] */
    {"^[]]$",       "]",                true},

    /* backtracking that only succeeds if star gives characters back:
       .* would swallow the whole line, so the final 'c' forces it to
       back off to the last 'c' present */
    {"^.*c$",       "abcabc",           true},
    {"^.*abc$",     "xxabcabc",         true},
};

typedef struct {
    const char* pattern;
    const char* expected_message;
} DbjGrepErrorCase;

static const DbjGrepErrorCase DBJ_GREP_ERROR_CASES[] = {
    {"*abc",  "* has nothing to repeat"},
    {"a**",   "* applied twice"},
    {"[abc",  "unterminated [ character class"},
    {"^*",    "* has nothing to repeat"},
};

/* The two length limits from dbj_grep.h, "LIMITS", exercised at the
   boundary. They need generated patterns rather than literals, so they
   are their own check rather than rows in the table above.

   Both limits are needed because they bound different things: a pattern
   of N literals is N instructions, but a pattern of character classes
   is far fewer instructions than characters -- which is exactly how a
   huge pattern could once slip past the capacity check untouched. */
static int run_limit_cases(void) {
    int failures = 0;

    /* one literal per character, so instruction count is the binding
       limit -- just over capacity must be refused */
    char over_capacity[DBJ_GREP_PATTERN_CAPACITY + 8];
    memset(over_capacity, 'a', sizeof over_capacity - 1);
    over_capacity[sizeof over_capacity - 1] = '\0';

    DbjGrepResult r = dbj_grep_prepare(over_capacity);
    if (r.tag != DBJ_GREP_PREPARE_ERR ||
        strcmp(r.err.message, "pattern too long") != 0) {
        failures++;
        SIMPLE_LOG("FAIL %zu literals: expected \"pattern too long\"",
                   sizeof over_capacity - 1);
    }

    /* one giant character class: few instructions, huge text -- the
       text limit is the only thing that can catch this */
    char over_text[DBJ_GREP_PATTERN_TEXT_MAX + 8];
    memset(over_text, 'x', sizeof over_text - 1);
    over_text[0] = '[';
    over_text[sizeof over_text - 2] = ']';
    over_text[sizeof over_text - 1] = '\0';

    r = dbj_grep_prepare(over_text);
    if (r.tag != DBJ_GREP_PREPARE_ERR ||
        strcmp(r.err.message, "pattern text too long") != 0) {
        failures++;
        SIMPLE_LOG("FAIL %zu-char class: expected \"pattern text too long\", got %s",
                   sizeof over_text - 1,
                   r.tag == DBJ_GREP_PREPARE_ERR ? r.err.message : "OK");
    }

    /* a pattern exactly at the text limit must still be accepted --
       an off-by-one here would reject legitimate input */
    char at_limit[DBJ_GREP_PATTERN_TEXT_MAX + 1];
    memset(at_limit, 'a', DBJ_GREP_PATTERN_TEXT_MAX);
    at_limit[DBJ_GREP_PATTERN_TEXT_MAX] = '\0';

    r = dbj_grep_prepare(at_limit);
    if (r.tag != DBJ_GREP_PREPARE_ERR ||
        strcmp(r.err.message, "pattern too long") != 0) {
        failures++;
        SIMPLE_LOG("FAIL text at exactly the limit should fail on capacity, not text");
    }

    /* null pattern is rejected, not dereferenced */
    r = dbj_grep_prepare(nullptr);
    if (r.tag != DBJ_GREP_PREPARE_ERR ||
        strcmp(r.err.message, "pattern is null") != 0) {
        failures++;
        SIMPLE_LOG("FAIL null pattern: expected \"pattern is null\"");
    }

    return failures;
}

/* Compiles the pattern and matches the line, requiring compilation to
   succeed -- a compile failure here is itself a test failure, reported
   through `ok`. Separated from run_self_test so the tagged-union branch
   stays one small readable switch instead of nesting inside the loop. */
static bool match_case_passes(const DbjGrepMatchCase* test_case) {
    bool retval = false;

    DbjGrepResult result = dbj_grep_prepare(test_case->pattern);

    switch (result.tag) {
        case DBJ_GREP_PREPARE_OK:
            retval = dbj_grep_match(&result.ok.pattern, test_case->line) ==
                     test_case->should_match;
            goto finish;
        case DBJ_GREP_PREPARE_ERR:
            SIMPLE_LOG("   compile failed at %s: %s",
                       result.err.location, result.err.message);
            retval = false;
            goto finish;
        // no default -- -Wswitch catches a missing tag case
    }

finish:
    return retval;
}

static int run_self_test(void) {
    int failures = 0;

    SIMPLE_LOG("match cases: %zu",
               sizeof DBJ_GREP_MATCH_CASES / sizeof DBJ_GREP_MATCH_CASES[0]);

    for (size_t i = 0; i < sizeof DBJ_GREP_MATCH_CASES / sizeof DBJ_GREP_MATCH_CASES[0]; i++) {
        const DbjGrepMatchCase* test_case = &DBJ_GREP_MATCH_CASES[i];
        if (!match_case_passes(test_case)) {
            failures++;
            SIMPLE_LOG("FAIL pattern \"%s\" against \"%s\", expected %s",
                       test_case->pattern, test_case->line,
                       test_case->should_match ? "match" : "no match");
        }
    }

    SIMPLE_LOG("error cases: %zu",
               sizeof DBJ_GREP_ERROR_CASES / sizeof DBJ_GREP_ERROR_CASES[0]);

    for (size_t i = 0; i < sizeof DBJ_GREP_ERROR_CASES / sizeof DBJ_GREP_ERROR_CASES[0]; i++) {
        const DbjGrepErrorCase* error_case = &DBJ_GREP_ERROR_CASES[i];
        DbjGrepResult result = dbj_grep_prepare(error_case->pattern);

        switch (result.tag) {
            case DBJ_GREP_PREPARE_OK:
                failures++;
                SIMPLE_LOG("FAIL pattern \"%s\" compiled, expected error \"%s\"",
                           error_case->pattern, error_case->expected_message);
                break;
            case DBJ_GREP_PREPARE_ERR:
                if (strcmp(result.err.message, error_case->expected_message) != 0) {
                    failures++;
                    SIMPLE_LOG("FAIL pattern \"%s\" gave \"%s\", expected \"%s\"",
                               error_case->pattern, result.err.message,
                               error_case->expected_message);
                }
                break;
            // no default -- -Wswitch catches a missing tag case
        }
    }

    SIMPLE_LOG("limit cases: pattern capacity %d, pattern text %d, files %d",
               DBJ_GREP_PATTERN_CAPACITY, DBJ_GREP_PATTERN_TEXT_MAX, GREP_MAX_FILES);
    failures += run_limit_cases();

    if (failures == 0)
        SIMPLE_LOG("all cases passed");
    else
        SIMPLE_LOG("%d case(s) FAILED", failures);

    return failures;
}

/* ── grep mode ──────────────────────────────────────────────────────
   Storage and parameters in, result out -- the original kept lnum,
   linebuf and the file descriptor in globals. */

static void grep_stream(const DbjGrepPattern* pattern, FILE* input,
                        const char* source_name, bool print_source_name) {
    char line[GREP_LINE_SIZE];

    while (fgets(line, sizeof line, input)) {
        size_t length = strlen(line);
        if (length > 0 && line[length - 1] == '\n')
            line[length - 1] = '\0';

        if (dbj_grep_match(pattern, line)) {
            if (print_source_name)
                printf("%s:", source_name);
            printf("%s\n", line);
        }
    }
}

static void print_usage(const char* program_name) {
    printf("Usage: %s <pattern> [file...]  -- grep, reading file(s) or stdin\n"
           "       %s --selftest           -- run the built-in self test\n"
           "       %s --help               -- print this text\n\n"
           "Supported pattern syntax (as in V6 grep):\n"
           "    .        any one character\n"
           "    *        zero or more of the preceding item\n"
           "    [set]    one character from set, ranges allowed (a-z)\n"
           "    [^set]   one character not in set\n"
           "    ^        start of line (only in first position)\n"
           "    $        end of line (only in last position)\n\n",
           program_name, program_name, program_name);
}

/* argv is declared with the C23 static + size-expression form required
   by CLAUDE.md, "Core principles" 9. main's second parameter has to
   stay compatible with char **, so it is an array of pointers -- and
   argc + 1 because argv[argc] is the terminating null pointer.

   --selftest and --help are matched before the pattern is compiled, so
   neither can be mistaken for one. Everything else is taken as the
   pattern: a bare `dbj_grep` with no arguments prints usage and stops
   rather than running the self test, since a no-argument invocation is
   far more often someone asking what this is than someone asking for
   the test suite. */
int main(int argc, char* argv[static argc + 1]) {
    int retval = 0;

    dbj_clintro("dbj_grep", "0.1.0");

    if (argc < 2) {
        print_usage(argv[0]);
        retval = 1;
        goto finish;
    }

    if (strcmp(argv[1], "--help") == 0) {
        print_usage(argv[0]);
        retval = 0;
        goto finish;
    }

    if (strcmp(argv[1], "--selftest") == 0) {
        retval = run_self_test() == 0 ? 0 : 1;
        goto finish;
    }

    DbjGrepResult compiled = dbj_grep_prepare(argv[1]);

    switch (compiled.tag) {
        case DBJ_GREP_PREPARE_ERR:
            fprintf(stderr, "dbj_grep: bad pattern \"%s\" -- %s (at %s)\n",
                    argv[1], compiled.err.message, compiled.err.location);
            retval = 2;
            goto finish;

        case DBJ_GREP_PREPARE_OK:
            if (argc < 3) {
                grep_stream(&compiled.ok.pattern, stdin, "(stdin)", false);
                retval = 0;
                goto finish;
            }

            int file_count = argc - 2;
            if (file_count > GREP_MAX_FILES) {
                fprintf(stderr,
                        "dbj_grep: %d files named, at most %d accepted\n",
                        file_count, GREP_MAX_FILES);
                retval = 2;
                goto finish;
            }

            /* More than one file named, so each match gets its source
               printed in front of it -- the original's `if (nfile > 0)`
               rule, kept. */
            bool print_source_name = file_count > 1;

            for (int i = 2; i < argc; i++) {
                FILE* input = fopen(argv[i], "r");
                if (!input) {
                    fprintf(stderr, "dbj_grep: can't open %s\n", argv[i]);
                    continue;
                }
                /* defer fires at the end of this loop body, so each
                   file is closed on its own iteration -- see CLAUDE.md,
                   "Core principles" 8, and corelib/dbj_defer.h. */
                defer { fclose(input); }

                grep_stream(&compiled.ok.pattern, input, argv[i], print_source_name);
            }
            retval = 0;
            goto finish;
        // no default -- -Wswitch catches a missing tag case
    }

finish:
    return retval;
}
