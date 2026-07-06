#pragma once
/*
    2026JUL06       (c) dbj@dbj.org

    EmailStorage — see dbj_discriminated_union.md.
    Requires dbj_email_record.h and dbj_email_storage_result.h to be
    included first.
*/

#ifndef U8TYPE
#define U8TYPE unsigned char
#endif

#ifndef EMAIL_STORAGE_CAPACITY
#define EMAIL_STORAGE_CAPACITY 0xF
#endif

typedef struct EmailStorage EmailStorage;

struct EmailStorage {
    EmailRecord records[EMAIL_STORAGE_CAPACITY];

    EmailStorageResult (*CreateEmail)(EmailStorage* self, EmailRecord record);
    EmailStorageResult (*ReadEmail)(EmailStorage* self, EmailId id);
    EmailStorageResult (*UpdateEmail)(EmailStorage* self, EmailRecord record);
    EmailStorageResult (*DeleteEmail)(EmailStorage* self, EmailId id);
};

/*
 * email_storage_instance() is the one factory callers use to obtain a
 * working EmailStorage — it zeroes the records array and wires up the
 * four CRUD function pointers. Callers never assign those pointers by
 * hand.
 */
EmailStorage email_storage_instance(void);

// define in exactly one translation unit (aka c file)
#ifdef DBJ_EMAIL_STORAGE_IMPLEMENTATION

#include <assert.h>
#include <string.h>

static EmailRecord* email_storage_find_by_id(EmailStorage* self, EmailId id) {
    assert(id != EMAIL_ID_EMPTY && "email_storage_find_by_id called with empty id");
    for (U8TYPE i = 0; i < EMAIL_STORAGE_CAPACITY; ++i)
        if (self->records[i].id == id) return &self->records[i];
    return nullptr;
}

static EmailRecord* email_storage_find_empty_slot(EmailStorage* self) {
    for (U8TYPE i = 0; i < EMAIL_STORAGE_CAPACITY; ++i)
        if (self->records[i].id == EMAIL_ID_EMPTY) return &self->records[i];
    return nullptr;
}

static EmailId email_storage_next_id = 0x01;

static EmailStorageResult email_storage_create(EmailStorage* self, EmailRecord record) {
    EmailRecord* slot = email_storage_find_empty_slot(self);
    if (!slot)
        return email_storage_result_err(__func__, "storage full");
    record.id = email_storage_next_id++;
    *slot = record;
    return email_storage_result_ok(*slot);
}

static EmailStorageResult email_storage_read(EmailStorage* self, EmailId id) {
    EmailRecord* found = email_storage_find_by_id(self, id);
    if (!found)
        return email_storage_result_err(__func__, "not found");
    return email_storage_result_ok(*found);
}

static EmailStorageResult email_storage_update(EmailStorage* self, EmailRecord record) {
    EmailRecord* slot = email_storage_find_by_id(self, record.id);
    if (!slot)
        return email_storage_result_err(__func__, "not found");
    *slot = record;
    return email_storage_result_ok(*slot);
}

static EmailStorageResult email_storage_delete(EmailStorage* self, EmailId id) {
    EmailRecord* slot = email_storage_find_by_id(self, id);
    if (!slot)
        return email_storage_result_err(__func__, "not found");
    EmailRecord deleted = *slot;
    *slot = EMPTY_EMAIL_RECORD;
    return email_storage_result_ok(deleted);
}

EmailStorage email_storage_instance(void) {
    EmailStorage storage = {0};
    storage.CreateEmail = email_storage_create;
    storage.ReadEmail   = email_storage_read;
    storage.UpdateEmail = email_storage_update;
    storage.DeleteEmail = email_storage_delete;
    return storage;
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
 *     EmailStorage db = email_storage_instance();
 *
 *     EmailStorageResult r = db.CreateEmail(&db, (EmailRecord){
 *         .to = "bob@example.com", .from = "alice@example.com",
 *         .subject = "hi", .body = "..."});
 *
 *     switch (r.tag) {
 *         case EMAIL_STORAGE_OK:  ... r.ok.record ...   break;
 *         case EMAIL_STORAGE_ERR: ... r.err.message ... break;
 *         // no default — -Wswitch catches a missing tag case
 *     }
 */
