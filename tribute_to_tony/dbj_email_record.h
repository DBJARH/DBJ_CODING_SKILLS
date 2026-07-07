#pragma once
/*
    2026JUL06       (c) dbj@dbj.org

    EmailRecord is the central, tagged type — see dbj_discriminated_union.md.
    Storage is a logical array of EmailRecord; the record's own id also
    serves as its index in that array.
*/

#include <stddef.h> // size_t

typedef size_t EmailId;

#define EMAIL_ID_EMPTY \
    ((EmailId)0x00) /* reserved — marks empty slot, NOT a null record */

#ifndef EMAIL_RECORD_TO_SIZE
#define EMAIL_RECORD_TO_SIZE 64
#endif

#ifndef EMAIL_RECORD_FROM_SIZE
#define EMAIL_RECORD_FROM_SIZE 64
#endif

#ifndef EMAIL_RECORD_SUBJECT_SIZE
#define EMAIL_RECORD_SUBJECT_SIZE 128
#endif

#ifndef EMAIL_RECORD_BODY_SIZE
#define EMAIL_RECORD_BODY_SIZE 512
#endif

typedef struct {
    // currently
    // record_id is simply index on the array of email records
    // held internally inside the email storage
    // if  that ever changes this still holds
    EmailId record_id;
    char to[EMAIL_RECORD_TO_SIZE];
    char from[EMAIL_RECORD_FROM_SIZE];
    char subject[EMAIL_RECORD_SUBJECT_SIZE];
    char body[EMAIL_RECORD_BODY_SIZE];
} EmailRecord;

static const EmailRecord EMPTY_EMAIL_RECORD = {.record_id = EMAIL_ID_EMPTY};
