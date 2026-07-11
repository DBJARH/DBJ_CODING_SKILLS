#pragma once
/*
    2026JUL06       (c) dbj@dbj.org

    EmailRecord is the central, tagged type — see dbj_discriminated_union.md.
    Storage is a logical array of EmailRecord; the record's own slot_id
    also serves as its index in that array. Occupancy (whether a slot
    holds a live record) is tracked separately by storage (see
    occupied[] on EmailStorage in dbj_email_storage.h), so there is no
    reserved "empty" id here -- every value of EmailId, including 0, can
    be a real record's slot_id.

    Two ids, two different lifetimes:
      - slot_id  -- the array index; reused by a different record once
                     its slot is freed and reissued (see the free list
                     in dbj_email_storage.h). What CRUD actually keys on.
      - unique_id -- a monotonic counter value assigned once on create,
                     never reused. Not used by CRUD/lookup anywhere; it
                     exists to make the slot-id-gets-reused behavior
                     legible -- read it back after a slot is reused and
                     it will differ from the id the previous occupant
                     had, even though slot_id is numerically identical.
*/

#include <stddef.h> // size_t

typedef size_t EmailId;

typedef size_t UniqueId;

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
    // slot_id is simply index on the array of email records held
    // internally inside the email storage -- reused after a delete, so
    // it is only a stable identity while the record is live (see
    // dbj_email_storage.h)
    EmailId slot_id;
    // unique_id is assigned once on create from a monotonic counter and
    // never reused, unlike slot_id -- storage does not use it for
    // lookup/CRUD, it exists only so a record's identity can be told
    // apart from another record
    UniqueId unique_id;
    char to[EMAIL_RECORD_TO_SIZE];
    char from[EMAIL_RECORD_FROM_SIZE];
    char subject[EMAIL_RECORD_SUBJECT_SIZE];
    char body[EMAIL_RECORD_BODY_SIZE];
} EmailRecord;
