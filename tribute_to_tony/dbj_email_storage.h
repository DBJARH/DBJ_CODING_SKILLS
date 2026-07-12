#pragma once
/*
    2026JUL06       (c) dbj@dbj.org

    EmailStorage — see general_design.md, section "EmailStorage"
    (and "Free Slots concept" for the create/delete slot-reuse flow).
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

/* No free-slot value can be a valid array index (indices are
   0..CAPACITY-1), so CAPACITY itself is safe to use as the "no free
   slot" sentinel on the free list. */
#define EMAIL_STORAGE_FREE_LIST_EMPTY EMAIL_STORAGE_CAPACITY

typedef struct EmailStorage EmailStorage;

struct EmailStorage {
    EmailRecord records[EMAIL_STORAGE_CAPACITY];

    /* Occupancy is tracked here, independently of what slot_id holds --
       so slot_id can be a plain 0-based slot index with no reserved
       "empty" value (see dbj_email_record.h). A deleted slot's index is
       pushed onto the free list and reissued to the next CreateEmail,
       so a slot_id is only a stable identity while occupied[id] is
       true: after delete, that same id value will be reused by
       whatever record lands in the slot next. next_free_slot[i] is
       only meaningful while slot i is on the free list (i.e.
       occupied[i] is false and i is reachable from free_list_head). */
    bool   occupied[EMAIL_STORAGE_CAPACITY];
    size_t next_free_slot[EMAIL_STORAGE_CAPACITY];
    size_t free_list_head;   /* EMAIL_STORAGE_FREE_LIST_EMPTY if none free */
    size_t high_water_mark;  /* count of slots ever occupied at least once */
    size_t live_count;       /* records currently in storage (create - delete) */

    EmailStorageResult (*CreateEmail)(EmailRecord record);
    EmailStorageResult (*ReadEmail)(EmailId id);
    EmailStorageResult (*UpdateEmail)(EmailRecord record);
    EmailStorageResult (*DeleteEmail)(EmailId id);
};

/*
 * create_email_storage_instance() wires the CRUD function pointers into
 * the single module-level EmailStorage and returns its address (see
 * "Notice on user required and assumed behavior" in general_design.md,
 * section "User / EmailStorage interaction") — every caller shares the
 * same records array, so a copy-by-value here would let each caller's
 * struct go stale the moment any CRUD verb runs against the singleton.
 * Callers never pass storage in by hand, so there is no `self`
 * parameter on any CRUD verb.
 *
 * Despite the name it is safe to call more than once (it just re-wires
 * the same function pointers), but callers should still only call it
 * once and hold onto the returned pointer -- see storage_instance() in
 * dbj_email_crud.c for the pattern.
 */
EmailStorage* create_email_storage_instance(void);

// define in exactly one translation unit (aka c file)
#ifdef DBJ_EMAIL_STORAGE_IMPLEMENTATION

#include <string.h>

/*
 * The EmailStorage synopsis in general_design.md drops `self` from
 * every CRUD verb, so the CRUD functions below close over this single
 * module-level instance instead of receiving storage as a parameter.
 * free_list_head starts at EMAIL_STORAGE_FREE_LIST_EMPTY (0-init would
 * wrongly mean "slot 0 is free"), so it is set explicitly in
 * create_email_storage_instance rather than relying on static
 * zero-initialization.
 */
static EmailStorage email_storage_singleton;

/*
 * slot_id is a plain 0-based slot index -- no reserved value, every
 * EmailId (including 0) can be a real record's slot_id. occupied[id] is
 * the sole source of truth for whether that slot currently holds a
 * live record.
 */
static EmailRecord* email_storage_find_by_id(EmailId id) {
    if (id >= EMAIL_STORAGE_CAPACITY) return nullptr;
    return email_storage_singleton.occupied[id] ? &email_storage_singleton.records[id] : nullptr;
}

/*
 * Counter is local-static, not a field on EmailStorage: it is unrelated
 * to slot reuse (it only ever increments, never rewound by delete), so
 * it does not belong next to free_list_head/high_water_mark/live_count,
 * which are all slot bookkeeping.
 */
static UniqueId next_unique_id(void) {
    static UniqueId next_ = 0;
    return next_++;
}

/*
 * Slot for the new record comes from the free list first (a delete's
 * leftover slot, reused so churn doesn't burn through capacity), and
 * only once that's empty from a never-before-used slot at
 * high_water_mark. Either way slot_id = slot index, so ids are reused
 * whenever their slot is: a slot_id is only a stable identity while
 * its record is live, not forever -- see the free list comment on
 * EmailStorage.
 *
 * unique_id is unrelated to slot reuse: it comes from next_unique_id(),
 * which only ever increments, so it stays distinct across the whole
 * run even when slot_id gets reissued to a different record. Nothing
 * in storage looks unique_id up by -- it is not an index into anything
 * -- so incoming record.unique_id (if the caller set one) is discarded
 * here in favor of the one storage assigns.
 */
static EmailStorageResult email_storage_create(EmailRecord record) {
    size_t slot = (size_t)0;
    if (email_storage_singleton.free_list_head != EMAIL_STORAGE_FREE_LIST_EMPTY) {
        slot = email_storage_singleton.free_list_head;
        email_storage_singleton.free_list_head = email_storage_singleton.next_free_slot[slot];
    } else if (email_storage_singleton.high_water_mark < EMAIL_STORAGE_CAPACITY) {
        slot = email_storage_singleton.high_water_mark++;
    } else {
        return email_storage_result_err(__func__, "storage full");
    }

    record.slot_id   = slot;
    record.unique_id = next_unique_id();
    email_storage_singleton.records[slot]  = record;
    email_storage_singleton.occupied[slot] = true;
    email_storage_singleton.live_count++;
    return email_storage_result_ok(record);
}

static EmailStorageResult email_storage_read(EmailId id) {
    EmailRecord* found = email_storage_find_by_id(id);
    if (!found)
        return email_storage_result_err(__func__, "not found");
    return email_storage_result_ok(*found);
}

/*
 * unique_id is assigned once, on create, and otherwise left alone: a
 * caller building `record` fresh (see update_n in dbj_email_crud.c)
 * has no reason to know the stored unique_id, so it is carried forward
 * from the existing slot here rather than taken from the incoming
 * record (which would otherwise zero it out on every update).
 */
static EmailStorageResult email_storage_update(EmailRecord record) {
    EmailRecord* slot = email_storage_find_by_id(record.slot_id);
    if (!slot)
        return email_storage_result_err(__func__, "not found");
    record.unique_id = slot->unique_id;
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
    email_storage_singleton.occupied[id] = false;

    email_storage_singleton.next_free_slot[id] = email_storage_singleton.free_list_head;
    email_storage_singleton.free_list_head = id;
    email_storage_singleton.live_count--;

    return email_storage_result_ok(deleted);
}

EmailStorage* create_email_storage_instance(void) {
    /* free_list_head's zero-init value (0) would wrongly mean "slot 0
       is free", so it needs one real initialization to
       EMAIL_STORAGE_FREE_LIST_EMPTY -- guarded by CreateEmail being
       unset, so a second call (still allowed, see the doc comment
       above the declaration) re-wires the function pointers without
       stomping on free-list state already built up by real CRUD use. */
    if (!email_storage_singleton.CreateEmail)
        email_storage_singleton.free_list_head = EMAIL_STORAGE_FREE_LIST_EMPTY;
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
 * Every other .c file just includes the header normally. Call
 * create_email_storage_instance() once and hold onto the returned
 * pointer -- see storage_instance() in dbj_email_crud.c for the
 * exactly-once pattern.
 *
 *     EmailStorage* db = create_email_storage_instance();
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
