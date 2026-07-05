#pragma once
/*
    2026APR04       (c) dbj@dbj.org
*/

// typedef unsigned char U8TYPE ;
// due to Code intellisense limitations we do the following
#ifndef U8TYPE
#define U8TYPE unsigned char
#endif

/* ── result (discriminated union #1) ──────────────────────────── */

typedef U8TYPE RetVAL; /* value carried in the ok arm of Result */

typedef enum : U8TYPE {
    OK,
    ERR,
} ResultTag;

typedef struct Result Result;

struct Result {
    ResultTag tag;
    union {
        struct {
            Result (*make)(RetVAL id);
            RetVAL id;
        } ok;
        struct {
            Result (*make)(const char* func, const char* msg);
            char msg[128];
        } err;
    };
};

/*
 * result_instance_ok/result_instance_err are the factory methods
 * callers use to obtain a Result. Each also wires the matching union
 * arm's make function pointer, so a Result already in hand can derive
 * another one of the same tag via ok.make / err.make.
 */
Result result_instance_ok(RetVAL id);
Result result_instance_err(const char* func, const char* msg);

// define in exactly one translation unit (aka c file)
#ifdef DBJ_ERR_IMPLEMENTATION

#include <assert.h>
#include <stdio.h>

Result result_instance_ok(RetVAL id) {
    return (Result){.tag = OK, .ok = {.make = result_instance_ok, .id = id}};
}

Result result_instance_err(const char* func, const char* msg) {
    assert(func && "result_instance_err called with null func");
    assert(msg && "result_instance_err called with null message");
    Result r = {.tag = ERR, .err = {.make = result_instance_err}};
    snprintf(r.err.msg, sizeof r.err.msg, "%s — %s", func, msg);
    return r;
}

#endif // DBJ_ERR_IMPLEMENTATION

/*
 * Typical usage
 * ─────────────
 * In exactly one .c file, define DBJ_ERR_IMPLEMENTATION before
 * including this header, so result_instance_ok/result_instance_err
 * get compiled in:
 *
 *     #define DBJ_ERR_IMPLEMENTATION
 *     #include "dbj_err.h"
 *
 * Every other .c file that needs the Result type just includes the
 * header normally (no #define) and gets declarations only:
 *
 *     #include "dbj_err.h"
 *
 * result_instance_ok/result_instance_err are factory methods — plain,
 * type-checked functions, one per tag, each returning a Result whose
 * matching union arm (ok or err) is wired up with the "methods" for
 * that arm — currently just make, but ok/err can each grow more
 * methods of their own without touching the other arm:
 *
 *     Result r = result_instance_ok(42);
 *     Result e = result_instance_err(__func__, "not found");
 *
 * From an existing Result, call make to derive another one of the same
 * tag without naming result_instance_ok/result_instance_err again:
 *
 *     Result r2 = r.ok.make(99);                  // tag == OK
 *     Result e2 = e.err.make(__func__, "again");  // tag == ERR
 *
 * Always branch on tag before touching ok/err — reading the inactive
 * arm of the union is undefined behaviour:
 *
 *     switch (r.tag) {
 *         case OK:  printf("id=%u\n", r.ok.id);   break;
 *         case ERR: printf("err=%s\n", r.err.msg); break;
 *         // no default — -Wswitch catches a missing ResultTag case
 *     }
 */
