#pragma once
/*
    2026JUL06       (c) dbj@dbj.org

    EmailRecord is the central, tagged type — see dbj_discriminated_union.md.
    Storage is a logical array of EmailRecord; the record's own id also
    serves as its index in that array.
*/

#ifndef U8TYPE
#define U8TYPE unsigned char
#endif

typedef U8TYPE EmailId;

#define EMAIL_ID_EMPTY \
    ((EmailId)0x00) /* reserved — marks empty slot, NOT a null record */

typedef struct {
    EmailId id;
    char to[64];
    char from[64];
    char subject[128];
    char body[512];
} EmailRecord;

static const EmailRecord EMPTY_EMAIL_RECORD = {.id = EMAIL_ID_EMPTY};
