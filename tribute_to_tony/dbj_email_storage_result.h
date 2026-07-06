#pragma once
/*
    2026JUL06       (c) dbj@dbj.org

    EmailStorageResult — see dbj_discriminated_union.md.
    Requires dbj_email_record.h (EmailRecord) to be included first.
*/

#ifndef U8TYPE
#define U8TYPE unsigned char
#endif

typedef enum : U8TYPE {
    EMAIL_STORAGE_OK,
    EMAIL_STORAGE_ERR,
} EmailStorageResultTag;

typedef struct EmailStorageResult EmailStorageResult;

struct EmailStorageResult {
    EmailStorageResultTag tag;
    union {
        struct {
            EmailStorageResult (*make)(EmailRecord record);
            EmailRecord record;
        } ok;
        struct {
            EmailStorageResult (*make)(const char* location, const char* message);
            char location[512]; /* error origin, e.g. via __func__ */
            char message[512];
        } err;
    };
};

/*
 * email_storage_result_ok/email_storage_result_err are the factory
 * methods callers use to obtain an EmailStorageResult. Each also wires
 * the matching union arm's make function pointer, so a result already
 * in hand can derive another one of the same tag via ok.make / err.make.
 */
EmailStorageResult email_storage_result_ok(EmailRecord record);
EmailStorageResult email_storage_result_err(const char* location, const char* message);

// define in exactly one translation unit (aka c file)
#ifdef DBJ_EMAIL_STORAGE_RESULT_IMPLEMENTATION

#include <stdio.h>

EmailStorageResult email_storage_result_ok(EmailRecord record) {
    return (EmailStorageResult){
        .tag = EMAIL_STORAGE_OK,
        .ok  = {.make = email_storage_result_ok, .record = record}};
}

EmailStorageResult email_storage_result_err(const char* location, const char* message) {
    EmailStorageResult r = {.tag = EMAIL_STORAGE_ERR,
                             .err = {.make = email_storage_result_err}};
    snprintf(r.err.location, sizeof r.err.location, "%s", location);
    snprintf(r.err.message, sizeof r.err.message, "%s", message);
    return r;
}

#endif // DBJ_EMAIL_STORAGE_RESULT_IMPLEMENTATION

/*
 * Typical usage
 * ─────────────
 * In exactly one .c file, define DBJ_EMAIL_STORAGE_RESULT_IMPLEMENTATION
 * before including this header, so the factory functions get compiled in:
 *
 *     #define DBJ_EMAIL_STORAGE_RESULT_IMPLEMENTATION
 *     #include "dbj_email_storage_result.h"
 *
 * Every other .c file just includes the header normally (no #define)
 * and gets declarations only:
 *
 *     #include "dbj_email_storage_result.h"
 *
 *     EmailStorageResult r = email_storage_result_ok(some_record);
 *     EmailStorageResult e = email_storage_result_err(__func__, "not found");
 *
 * From an existing result, call make to derive another one of the same
 * tag without naming the factory function again:
 *
 *     EmailStorageResult r2 = r.ok.make(other_record);   // tag == EMAIL_STORAGE_OK
 *     EmailStorageResult e2 = e.err.make(loc, "again");  // tag == EMAIL_STORAGE_ERR
 *
 * Always branch on tag before touching ok/err — reading the inactive
 * arm of the union is undefined behaviour:
 *
 *     switch (r.tag) {
 *         case EMAIL_STORAGE_OK:  ... r.ok.record ...  break;
 *         case EMAIL_STORAGE_ERR: ... r.err.message ... break;
 *         // no default — -Wswitch catches a missing tag case
 *     }
 */
