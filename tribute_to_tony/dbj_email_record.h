#pragma once
/*
    2026JUL06       (c) dbj@dbj.org

    EmailRecord is the central, tagged type — see dbj_discriminated_union.md.
    Storage is a logical array of EmailRecord; the record's own id also
    serves as its index in that array.
*/

#include <stddef.h> // size_t

#ifndef U8TYPE
#define U8TYPE unsigned char
#endif

typedef size_t EmailId;

#define EMAIL_ID_EMPTY \
    ((EmailId)0x00) /* reserved — marks empty slot, NOT a null record */

typedef struct {
    // currently
    // record_id is simply index on the array of email records
    // held internally inside the email storage
    // if  that ever changes this still holds
    EmailId record_id;
    char to[64];
    char from[64];
    char subject[128];
    char body[512];
} EmailRecord;

static const EmailRecord EMPTY_EMAIL_RECORD = {.record_id = EMAIL_ID_EMPTY};
