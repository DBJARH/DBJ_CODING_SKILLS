#pragma once
/*
    2026JUL06       (c) dbj@dbj.org

    EmailStorageResult — see general_design.md, section
    "EmailStorageResult".
    Requires dbj_email_record.h (EmailRecord) to be included first.
*/

typedef enum : unsigned short {
    EMAIL_STORAGE_OK,
    EMAIL_STORAGE_ERR,
} EmailStorageResultTag;

typedef struct EmailStorageResult EmailStorageResult;

#ifndef ERROR_RECORD_LOCATION_SIZE
#define ERROR_RECORD_LOCATION_SIZE 512
#endif

#ifndef ERROR_RECORD_MESSAGE_SIZE
#define ERROR_RECORD_MESSAGE_SIZE 512
#endif

/*
 * The err arm is its own named type, not an anonymous struct inside
 * the union, for the same reason EmailRecord is: both arms are then
 * single values with names, which makes each factory take exactly one
 * argument -- and that is what lets _Generic pick between them (see
 * email_storage_result, below).
 */
typedef struct {
    char location[ERROR_RECORD_LOCATION_SIZE]; /* error origin, e.g. via __func__ */
    char message[ERROR_RECORD_MESSAGE_SIZE];
} ErrorRecord;

/*
 * No function pointer lives in this struct, in the union or beside it
 * -- see dbj_discriminated_union_reference_implementation.md, section
 * "Why not embed a function pointer". Briefly: it would duplicate what
 * tag already says (and could then disagree with it), cost every
 * result an extra pointer, and buy back an extensibility a closed
 * two-arm union does not want. Dispatch is the switch on tag, below.
 */
struct EmailStorageResult {
    EmailStorageResultTag tag;
    union {
        EmailRecord email;
        ErrorRecord error;
    };
};

/*
 * error_record is the factory for the err arm's payload. Callers who
 * want the two-string form use it inline:
 *
 *     email_storage_result(error_record(__func__, "not found"))
 */
ErrorRecord error_record(const char* location, const char* message);

/*
 * email_storage_result_ok/email_storage_result_err are the factory
 * methods that make an EmailStorageResult. They are the only way a
 * result is made, so tag and the active union arm cannot fall out of
 * step.
 */
EmailStorageResult email_storage_result_ok(EmailRecord email);
EmailStorageResult email_storage_result_err(ErrorRecord error);

/*
 * Both arms are one named type each, so the compile-time type of the
 * payload already says which arm it is -- _Generic reads that and
 * picks the factory, so no caller ever names a tag. See section
 * "_Generic -- the reference implementation" in
 * dbj_discriminated_union_reference_implementation.md.
 *
 * A payload of any other type has no association here and fails to
 * compile, which is the point.
 */
#define email_storage_result(val) _Generic((val), \
    EmailRecord: email_storage_result_ok,         \
    ErrorRecord: email_storage_result_err)(val)

// define in exactly one translation unit (aka c file)
#ifdef DBJ_EMAIL_STORAGE_RESULT_IMPLEMENTATION

#include <stdio.h>

ErrorRecord error_record(const char* location, const char* message) {
    ErrorRecord rec = {0};
    snprintf(rec.location, sizeof rec.location, "%s", location);
    snprintf(rec.message, sizeof rec.message, "%s", message);
    return rec;
}

EmailStorageResult email_storage_result_ok(EmailRecord email) {
    return (EmailStorageResult){.tag = EMAIL_STORAGE_OK, .email = email};
}

EmailStorageResult email_storage_result_err(ErrorRecord error) {
    return (EmailStorageResult){.tag = EMAIL_STORAGE_ERR, .error = error};
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
 * Make a result through the _Generic macro -- the payload's type
 * selects the arm, so no tag is ever written by hand:
 *
 *     EmailStorageResult r = email_storage_result(some_record);
 *     EmailStorageResult e = email_storage_result(
 *                                error_record(__func__, "not found"));
 *
 * The two factories are also callable directly, if naming the arm at
 * the call site reads better there.
 *
 * Always branch on tag before touching email/error — reading the
 * inactive arm of the union is undefined behaviour:
 *
 *     switch (r.tag) {
 *         case EMAIL_STORAGE_OK:  ... r.email ...          break;
 *         case EMAIL_STORAGE_ERR: ... r.error.message ...  break;
 *         // no default — -Wswitch catches a missing tag case
 *     }
 */
