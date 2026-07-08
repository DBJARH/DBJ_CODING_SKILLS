#pragma once
/*
    2026JUL06       (c) dbj@dbj.org

    EmailStorage — see dbj_discriminated_union.md.
    Requires dbj_email_record.h and dbj_email_storage_result.h to be
    included first.
*/

#ifndef EMAIL_STORAGE_CAPACITY
#define EMAIL_STORAGE_CAPACITY 1024 * 1024
#endif

/* Operational boundaries for any runtime-configured email count (e.g.
   NUMBER_OF_EMAILS from an ini file) -- callers should validate against
   these before creating that many emails, rather than discovering the
   ceiling only when CreateEmail starts returning "storage full". */
#define MIN_EMAILS_STORAGE_LIMIT 1
#define MAX_EMAILS_STORAGE_LIMIT EMAIL_STORAGE_CAPACITY

typedef struct EmailStorage EmailStorage;

struct EmailStorage {
    EmailRecord records[EMAIL_STORAGE_CAPACITY];

    EmailStorageResult (*CreateEmail)(EmailRecord record);
    EmailStorageResult (*ReadEmail)(EmailId id);
    EmailStorageResult (*UpdateEmail)(EmailRecord record);
    EmailStorageResult (*DeleteEmail)(EmailId id);
};

/*
 * email_storage_instance() is the one factory callers use to obtain a
 * working EmailStorage. It returns a pointer to a single module-level
 * storage instance (see "Notice on user required and assumed behavior"
 * in dbj_discriminated_union.md) — every caller shares the same
 * records array, so a copy-by-value here would let each caller's
 * struct go stale the moment any CRUD verb runs against the singleton.
 * Callers never pass storage in by hand, so there is no `self`
 * parameter on any CRUD verb.
 */
EmailStorage* email_storage_instance(void);

// define in exactly one translation unit (aka c file)
#ifdef DBJ_EMAIL_STORAGE_IMPLEMENTATION

#include <assert.h>
#include <string.h>

/*
 * Synopsys 1 (dbj_discriminated_union.md) drops `self` from every CRUD
 * verb, so the CRUD functions below close over this single
 * module-level instance instead of receiving storage as a parameter.
 */
static EmailStorage email_storage_singleton;

/*
 * record_id is the array index, one-based (0 is EMAIL_ID_EMPTY). No
 * empty-slot scan is needed: email_storage_next_id only ever grows, so
 * a deleted slot is retired, never handed back out — see the "no
 * reuse" call in dbj_discriminated_union.md's discussion.
 * 
 * DBJ 2026-07-07
 * If and when we change to re use freed slots we will maintain a 
 * single linked list of free slots, again faster than walking from 0
 * trying to find empty slot
 */
static EmailRecord* email_storage_find_by_id(EmailId id) {
    assert(id != EMAIL_ID_EMPTY && "email_storage_find_by_id called with empty id");
    if (id > EMAIL_STORAGE_CAPACITY) return nullptr;
    EmailRecord* slot = &email_storage_singleton.records[id - 1];
    return slot->record_id == id ? slot : nullptr;
}

static EmailId email_storage_next_id = 0x01;

static EmailStorageResult email_storage_create(EmailRecord record) {
    if (email_storage_next_id > EMAIL_STORAGE_CAPACITY)
        return email_storage_result_err(__func__, "storage full");
    record.record_id = email_storage_next_id++;
    EmailRecord* slot = &email_storage_singleton.records[record.record_id - 1];
    *slot = record;
    return email_storage_result_ok(*slot);
}

static EmailStorageResult email_storage_read(EmailId id) {
    EmailRecord* found = email_storage_find_by_id(id);
    if (!found)
        return email_storage_result_err(__func__, "not found");
    return email_storage_result_ok(*found);
}

static EmailStorageResult email_storage_update(EmailRecord record) {
    EmailRecord* slot = email_storage_find_by_id(record.record_id);
    if (!slot)
        return email_storage_result_err(__func__, "not found");
    *slot = record;
    return email_storage_result_ok(*slot);
}

/**
 * DBJ 2026-07-07 
 * on success we return an deleted record
 */
static EmailStorageResult email_storage_delete(EmailId id) {
    EmailRecord* slot = email_storage_find_by_id(id);
    if (!slot)
        return email_storage_result_err(__func__, "not found");
    EmailRecord deleted = *slot;
    *slot = EMPTY_EMAIL_RECORD;
    return email_storage_result_ok(deleted);
}

EmailStorage* email_storage_instance(void) {
    email_storage_singleton.CreateEmail = email_storage_create;
    email_storage_singleton.ReadEmail   = email_storage_read;
    email_storage_singleton.UpdateEmail = email_storage_update;
    email_storage_singleton.DeleteEmail = email_storage_delete;
    return &email_storage_singleton;
}

#endif // DBJ_EMAIL_STORAGE_IMPLEMENTATION

/*
 * Typical usage
 * ─────────────
 * In exactly one .c file, define DBJ_EMAIL_STORAGE_IMPLEMENTATION
 * before including this header:
 *
 *     #define DBJ_EMAIL_STORAGE_IMPLEMENTATION
 *     #include "dbj_email_storage.h"
 *
 * Every other .c file just includes the header normally.
 *
 *     EmailStorage* db = email_storage_instance();
 *
 *     EmailStorageResult r = db->CreateEmail((EmailRecord){
 *         .to = "bob@example.com", .from = "alice@example.com",
 *         .subject = "hi", .body = "..."});
 *
 *     switch (r.tag) {
 *         case EMAIL_STORAGE_OK:  ... r.ok.record ...   break;
 *         case EMAIL_STORAGE_ERR: ... r.err.message ... break;
 *         // no default — -Wswitch catches a missing tag case
 *     }
 */
