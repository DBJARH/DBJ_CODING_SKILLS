#pragma once
/*
    2026JUL30       (c) dbj@dbj.org

    DbjGrepInstruction / DbjGrepPattern / DbjGrepResult -- see
    readme.md, sections "DbjGrepInstruction", "DbjGrepPattern" and
    "DbjGrepResult".

    The C23 tagged union port of the opcode encoding in Ken Thompson's
    Version 6 Unix grep (c. 1975). The original packed opcodes and their
    operands into one flat char expbuf[512], where every opcode had a
    different operand width and nothing in the type system recorded
    that -- the reader simply had to know CCHR took one following byte,
    CCL a length-prefixed set, CDOT nothing at all. Here each
    instruction is a tag plus its own payload, so the next instruction
    is always at + 1 and never at ep += *ep.

    Single header (STB) style: declaration-only unless
    DBJ_GREP_IMPLEMENTATION is defined first.
*/

#include <dbj_required_compile_time.h>

#include <stddef.h> // size_t

/*
    LIMITS
    ------
    Every buffer here is fixed size, nothing is allocated, and every
    limit below is checked at the one place input can exceed it -- an
    over-limit input is always a DBJ_GREP_PREPARE_ERR carrying which
    limit it hit, never a truncation and never undefined behaviour.

    This carries over the original's expbuf[512] constraint honestly
    rather than quietly replacing it with a growable buffer.

    DBJ_GREP_PATTERN_CAPACITY bounds the *instruction count*, which is
    not the same as the pattern's length in characters: "[a-z]" is five
    characters but one instruction, while "abcde" is five of each. Both
    have to be bounded, so DBJ_GREP_PATTERN_TEXT_MAX below bounds the
    text independently -- without it a pattern made of character
    classes could be arbitrarily long while producing few enough
    instructions to slip under the capacity check.
*/
#ifndef DBJ_GREP_PATTERN_CAPACITY
#define DBJ_GREP_PATTERN_CAPACITY 256
#endif

/* Longest pattern text accepted, in characters. Generous next to
   anything written by hand -- the point is that some bound exists and
   is reported, not that it is tight. */
#ifndef DBJ_GREP_PATTERN_TEXT_MAX
#define DBJ_GREP_PATTERN_TEXT_MAX 1024
#endif

/* Deliberately a literal 512 and not BUFSIZ, unlike the driver's line
   buffer: this is a field inside every DbjGrepResult, not a
   stdio read buffer, so tying it to the platform's stdio buffer size
   would be a false connection -- and an expensive one, since BUFSIZ is
   implementation-defined (8192 on glibc) and this size is paid twice
   per result value. */
#ifndef DBJ_GREP_ERROR_TEXT_SIZE
#define DBJ_GREP_ERROR_TEXT_SIZE 512
#endif

/* One entry per possible unsigned char value, for DBJ_GREP_CHAR_CLASS's
   membership table. The original scanned a length-prefixed byte list
   per input character instead (its cclass()); the table is O(1) per
   character and, more to the point here, has one obvious shape rather
   than an implicit length convention. */
#define DBJ_GREP_CHAR_CLASS_SIZE 256

/*
    The opcode tag. Each name is followed by the V6 opcode it replaces.

    DBJ_GREP_CHAR_CLASS covers both of the original's CCL and NCCL: those
    two differ only in the sense of the test, so the difference is a
    `negated` bool on the payload rather than a second tag.

    The original's CSTAR has no counterpart here at all -- see the
    `star` field on DbjGrepInstruction below, and readme.md,
    "DbjGrepInstruction", for why it stopped being an opcode.
*/
typedef enum : unsigned short {
    DBJ_GREP_LITERAL,     /* CCHR  -- match one specific character      */
    DBJ_GREP_ANY,         /* CDOT  -- match any one character           */
    DBJ_GREP_CHAR_CLASS,  /* CCL / NCCL -- match one character from set */
    DBJ_GREP_LINE_END,    /* CDOL  -- match only at end of line         */
    DBJ_GREP_HALT,        /* CEOF  -- whole pattern matched             */
} DbjGrepOpcode;

typedef struct {
    DbjGrepOpcode opcode;
    /* This instruction repeats zero or more times -- the original wrote
       a separate CSTAR opcode in front of the item being repeated. Only
       ever set on the three consuming opcodes (LITERAL, ANY,
       CHAR_CLASS); dbj_grep_prepare rejects `*` after anything else. */
    bool star;
    union {
        struct {
            char value;
        } literal;
        struct {
            /* member[c] is true if unsigned char c is in the set;
               negated flips the sense of the whole test */
            bool member[DBJ_GREP_CHAR_CLASS_SIZE];
            bool negated;
        } char_class;
        /* DBJ_GREP_ANY, DBJ_GREP_LINE_END and DBJ_GREP_HALT carry no payload --
           the tag alone is the whole instruction */
    };
} DbjGrepInstruction;

typedef struct {
    DbjGrepInstruction code[DBJ_GREP_PATTERN_CAPACITY];
    size_t length;
    /* Pattern began with ^, so a match is tried at line start only.
       This is the original's circf flag. It is a property of the whole
       pattern rather than of any single instruction, so it lives here
       and not on DbjGrepInstruction. */
    bool anchored;
} DbjGrepPattern;

typedef enum : unsigned short {
    DBJ_GREP_PREPARE_OK,
    DBJ_GREP_PREPARE_ERR,
} DbjGrepPrepareTag;

/*
    DbjGrepResult is what dbj_grep_prepare hands back: either a
    finished DbjGrepPattern or the reason there isn't one. It is the
    second tagged union in this header, and its purpose is different
    from DbjGrepInstruction's.

    (V6 called this step "compiling" the expression -- its function is
    compile(). Nothing is compiled in the modern sense: a pattern string
    is read once into an instruction array, so the operation is named
    prepare here. See readme.md, "Prepare".)

    DbjGrepInstruction models the *data* -- one opcode and the payload
    that belongs to it. DbjGrepResult models the *outcome* of a
    fallible operation, so that "it worked, here is the pattern" and "it
    failed, here is why" are one value with one tag, rather than a
    sentinel return plus an out-parameter plus a convention about which
    to read first.

    The point is that the caller cannot reach either arm without first
    branching on `tag`. A bad pattern is therefore impossible to use as
    if it were a good one -- there is no half-built pattern to
    accidentally run, and no errno-style global to forget to check. The
    original had neither: its compile() wrote into the global expbuf and
    called error() to exit the process outright, so a caller wanting to
    survive a bad pattern had nowhere to put that fact.

    Same shape as tribute_to_tony/dbj_email_storage_result.h, and for
    the same reason -- see readme.md, "DbjGrepResult".
*/
typedef struct DbjGrepResult DbjGrepResult;

struct DbjGrepResult {
    DbjGrepPrepareTag tag;
    union {
        struct {
            DbjGrepResult (*make)(DbjGrepPattern pattern);
            DbjGrepPattern pattern;
        } ok;
        struct {
            DbjGrepResult (*make)(const char* location, const char* message);
            char location[DBJ_GREP_ERROR_TEXT_SIZE]; /* error origin, e.g. via __func__ */
            char message[DBJ_GREP_ERROR_TEXT_SIZE];
        } err;
    };
};

/*
 * dbj_grep_result_ok/dbj_grep_result_err are the factory
 * methods callers use to obtain a DbjGrepResult. Each also wires
 * the matching union arm's make function pointer, so a result already
 * in hand can derive another one of the same tag via ok.make / err.make.
 * Same shape as tribute_to_tony/dbj_email_storage_result.h, on purpose.
 */
DbjGrepResult dbj_grep_result_ok(DbjGrepPattern pattern);
DbjGrepResult dbj_grep_result_err(const char* location, const char* message);

/* Prepare pattern text into a DbjGrepPattern. Storage and parameters in,
   result out -- the original wrote into the global expbuf and returned
   nothing. */
DbjGrepResult dbj_grep_prepare(const char* pattern);

/* True if line matches pattern anywhere (or at line start only, when
   pattern.anchored). */
bool dbj_grep_match(const DbjGrepPattern* pattern, const char* line);

// define in exactly one translation unit (aka c file)
#ifdef DBJ_GREP_IMPLEMENTATION

#include <stdio.h>
#include <string.h>

DbjGrepResult dbj_grep_result_ok(DbjGrepPattern pattern) {
    return (DbjGrepResult){
        .tag = DBJ_GREP_PREPARE_OK,
        .ok  = {.make = dbj_grep_result_ok, .pattern = pattern}};
}

DbjGrepResult dbj_grep_result_err(const char* location, const char* message) {
    DbjGrepResult r = {.tag = DBJ_GREP_PREPARE_ERR,
                            .err = {.make = dbj_grep_result_err}};
    snprintf(r.err.location, sizeof r.err.location, "%s", location);
    snprintf(r.err.message, sizeof r.err.message, "%s", message);
    return r;
}

/*
    Read one [...] character class out of the pattern.

    `cursor` points just past the '[' on entry and is left just past the
    ']' on exit. Returns false if the class is unterminated, which is
    the one error this can produce.

    Ranges (a-z) are handled here; the original V6 compiler did not have
    them, but a class without ranges is tedious enough to write that
    leaving them out would make the POC harder to exercise, not simpler.
*/
static bool dbj_grep_read_char_class(const char** cursor, DbjGrepInstruction* instruction) {
    const char* scan = *cursor;

    *instruction = (DbjGrepInstruction){.opcode = DBJ_GREP_CHAR_CLASS};

    if (*scan == '^') {
        instruction->char_class.negated = true;
        scan++;
    }

    /* A ']' in the very first position is a literal ']', not the
       terminator -- the long-standing convention, and it is why this is
       a do/while shape rather than a plain while. */
    bool first = true;
    while (*scan && (*scan != ']' || first)) {
        unsigned char low = (unsigned char)*scan++;

        if (*scan == '-' && scan[1] && scan[1] != ']') {
            unsigned char high = (unsigned char)scan[1];
            scan += 2;
            if (low > high) {
                unsigned char swap = low;
                low = high;
                high = swap;
            }
            for (unsigned int code = low; code <= high; code++)
                instruction->char_class.member[code] = true;
        } else {
            instruction->char_class.member[low] = true;
        }
        first = false;
    }

    if (*scan != ']')
        return false;

    *cursor = scan + 1; /* step past the ']' */
    return true;
}

DbjGrepResult dbj_grep_prepare(const char* pattern) {
    if (pattern == nullptr)
        return dbj_grep_result_err(__func__, "pattern is null");

    /* Bound the text before walking it. strnlen, not strlen: a pattern
       longer than the limit is rejected without reading past it, so an
       unterminated argument cannot run away. */
    if (strnlen(pattern, DBJ_GREP_PATTERN_TEXT_MAX + 1) > DBJ_GREP_PATTERN_TEXT_MAX)
        return dbj_grep_result_err(__func__, "pattern text too long");

    DbjGrepPattern prepared = {};
    const char* scan = pattern;

    if (*scan == '^') {
        prepared.anchored = true;
        scan++;
    }

    while (*scan) {
        /* '*' emits nothing -- it reaches back and sets star on the
           instruction just emitted. The mirror image of the original
           writing CSTAR *before* its item, and unlike that encoding it
           cannot produce a star with nothing to repeat. */
        if (*scan == '*') {
            if (prepared.length == 0)
                return dbj_grep_result_err(__func__, "* has nothing to repeat");

            DbjGrepInstruction* previous = &prepared.code[prepared.length - 1];
            switch (previous->opcode) {
                case DBJ_GREP_LITERAL:
                case DBJ_GREP_ANY:
                case DBJ_GREP_CHAR_CLASS:
                    if (previous->star)
                        return dbj_grep_result_err(__func__, "* applied twice");
                    previous->star = true;
                    break;
                case DBJ_GREP_LINE_END:
                case DBJ_GREP_HALT:
                    return dbj_grep_result_err(__func__, "* has nothing to repeat");
            }
            scan++;
            continue;
        }

        /* One slot is always kept in reserve for the DBJ_GREP_HALT emitted
           after this loop, hence the - 1. */
        if (prepared.length >= DBJ_GREP_PATTERN_CAPACITY - 1)
            return dbj_grep_result_err(__func__, "pattern too long");

        DbjGrepInstruction instruction = {};

        if (*scan == '.') {
            instruction.opcode = DBJ_GREP_ANY;
            scan++;
        } else if (*scan == '[') {
            scan++; /* step past the '[' */
            if (!dbj_grep_read_char_class(&scan, &instruction))
                return dbj_grep_result_err(__func__, "unterminated [ character class");
        } else if (*scan == '$' && scan[1] == '\0') {
            /* '$' is an anchor only in final position; anywhere else it
               is an ordinary character. Same rule as the original. */
            instruction.opcode = DBJ_GREP_LINE_END;
            scan++;
        } else {
            instruction.opcode = DBJ_GREP_LITERAL;
            instruction.literal.value = *scan;
            scan++;
        }

        prepared.code[prepared.length++] = instruction;
    }

    prepared.code[prepared.length++] = (DbjGrepInstruction){.opcode = DBJ_GREP_HALT};

    return dbj_grep_result_ok(prepared);
}

/*
    Does one instruction match the single character `input`?

    Split out of dbj_grep_advance below so the greedy/backtracking star
    loop and the plain single-step path can share one answer, instead of
    the original's arrangement where each opcode case re-tested the
    character itself.

    Never called with input == '\0': end of line consumes nothing, so
    both call sites check for it first.
*/
static bool dbj_grep_instruction_accepts(const DbjGrepInstruction* instruction, char input) {
    switch (instruction->opcode) {
        case DBJ_GREP_LITERAL:
            return instruction->literal.value == input;
        case DBJ_GREP_ANY:
            return true;
        case DBJ_GREP_CHAR_CLASS:
            return instruction->char_class.member[(unsigned char)input] !=
                   instruction->char_class.negated;
        case DBJ_GREP_LINE_END:
        case DBJ_GREP_HALT:
            /* Neither consumes a character, so neither can be asked
               whether it accepts one. Reaching here means dbj_grep_advance
               dispatched the wrong way. */
            return false;
    }
    return false; /* unreachable: the switch above is exhaustive over
                     DbjGrepOpcode, but GCC still wants a return here */
}

/*
    Walk the instruction stream against the line. Recursive on the
    starred case, exactly as the 1975 advance() was -- see
    readme.md, "Match", on why that is kept rather than fixed.

    This is the exhaustive switch the whole port exists for. There is
    deliberately no default: adding an opcode to DbjGrepOpcode without
    handling it here is then a compile error under -Wswitch -Werror.
*/
static bool dbj_grep_advance(const DbjGrepInstruction* code, const char* line) {
    for (;;) {
        switch (code->opcode) {
            case DBJ_GREP_HALT:
                return true;

            case DBJ_GREP_LINE_END:
                /* Consumes nothing -- it is a position, not a
                   character. Cannot be starred (dbj_grep_prepare rejects
                   that), so there is no star handling in this arm. */
                if (*line != '\0')
                    return false;
                code++;
                continue;

            case DBJ_GREP_LITERAL:
            case DBJ_GREP_ANY:
            case DBJ_GREP_CHAR_CLASS:
                if (code->star) {
                    /* Greedy: run forward while the item keeps
                       matching, then give characters back one at a time
                       and try the rest of the pattern at each position.
                       The original ran to end of line unconditionally
                       (while (*lp++)) and backtracked from there, which
                       only terminated correctly because its starred
                       item was always exactly one character wide. */
                    const char* furthest = line;
                    while (*furthest && dbj_grep_instruction_accepts(code, *furthest))
                        furthest++;

                    /* >= line, not > line: zero repetitions is a valid
                       match for star, so the starting position has to
                       be tried too. */
                    for (const char* back = furthest; back >= line; back--)
                        if (dbj_grep_advance(code + 1, back))
                            return true;

                    return false;
                }

                if (*line == '\0' || !dbj_grep_instruction_accepts(code, *line))
                    return false;
                line++;
                code++;
                continue;
        }
    }
}

bool dbj_grep_match(const DbjGrepPattern* pattern, const char* line) {
    if (pattern == nullptr || line == nullptr)
        return false;

    if (pattern->anchored)
        return dbj_grep_advance(pattern->code, line);

    /* Unanchored: try every starting position, including the one past
       the last character -- an empty pattern, or one that is all stars,
       matches there. do/while rather than while so that position is
       reached even when line is empty. */
    const char* start = line;
    do {
        if (dbj_grep_advance(pattern->code, start))
            return true;
    } while (*start++);

    return false;
}

#endif // DBJ_GREP_IMPLEMENTATION

/*
 * Typical usage
 * ─────────────
 * In exactly one .c file, define DBJ_GREP_IMPLEMENTATION
 * before including this header, so the functions get compiled in:
 *
 *     #define DBJ_GREP_IMPLEMENTATION
 *     #include "dbj_grep.h"
 *
 * Every other .c file just includes the header normally (no #define)
 * and gets declarations only.
 *
 * Always branch on tag before touching ok/err -- reading the inactive
 * arm of the union is undefined behaviour:
 *
 *     DbjGrepResult r = dbj_grep_prepare("^a.[0-9]*$");
 *     switch (r.tag) {
 *         case DBJ_GREP_PREPARE_OK:
 *             if (dbj_grep_match(&r.ok.pattern, line)) { ... }
 *             break;
 *         case DBJ_GREP_PREPARE_ERR:
 *             ... r.err.location, r.err.message ...
 *             break;
 *         // no default -- -Wswitch catches a missing tag case
 *     }
 */
