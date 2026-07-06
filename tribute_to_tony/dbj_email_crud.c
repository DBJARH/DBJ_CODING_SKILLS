/*
    2026JUL06       (c) dbj@dbj.org

    gcc -std=c2x -Wall -Wswitch -Werror -o dbj_email_crud dbj_email_crud.c

    See dbj_discriminated_union.md for the design this implements.
    Single header (STB) style: dbj_email_record.h, dbj_email_storage_result.h
    and dbj_email_storage.h are declaration-only unless their
    *_IMPLEMENTATION macro is defined first, as done below.
*/
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#define U8TYPE unsigned char

#include "dbj_email_record.h"

#define DBJ_EMAIL_STORAGE_RESULT_IMPLEMENTATION
#include "dbj_email_storage_result.h"

#define DBJ_EMAIL_STORAGE_IMPLEMENTATION
#include "dbj_email_storage.h"

#include "dbj_log.h"

int main(void) {
    my_LOG("=== dbj email CRUD — RQ01 ===");
    my_LOG("===  2026JUL06  (c) dbj@dbj.org   ===");

    EmailStorage db = email_storage_instance();

    /* CREATE */
    EmailStorageResult c1 = db.CreateEmail(
        &db, (EmailRecord){.to = "bob@example.com", .from = "alice@example.com",
                            .subject = "Q1 report", .body = "Please find attached..."});
    assert(c1.tag == EMAIL_STORAGE_OK && "CREATE bob failed");
    EmailId bob_id = c1.ok.record.id;

    EmailStorageResult c2 = db.CreateEmail(
        &db, (EmailRecord){.to = "carol@example.com", .from = "alice@example.com",
                            .subject = "Meeting notes", .body = "As discussed..."});
    assert(c2.tag == EMAIL_STORAGE_OK && "CREATE carol failed");
    EmailId carol_id = c2.ok.record.id;

    /* READ */
    EmailStorageResult r1 = db.ReadEmail(&db, carol_id);
    if (r1.tag == EMAIL_STORAGE_OK)
        my_LOG("READ  id=%u subject=%s", r1.ok.record.id, r1.ok.record.subject);
    else
        my_LOG("READ  id=%u failed — %s", carol_id, r1.err.message);

    EmailStorageResult r2 = db.ReadEmail(&db, 0x42);
    assert(r2.tag == EMAIL_STORAGE_ERR && "READ 0x42 must fail — id never created");
    (void)r2;

    /* UPDATE */
    EmailStorageResult u1 = db.UpdateEmail(
        &db, (EmailRecord){.id = bob_id, .to = "bob@example.com",
                            .from = "alice@example.com",
                            .subject = "Q1 report (final)", .body = "Revised."});
    if (u1.tag == EMAIL_STORAGE_OK)
        my_LOG("UPDATE id=%u subject=%s", u1.ok.record.id, u1.ok.record.subject);
    else
        my_LOG("UPDATE id=%u failed — %s", bob_id, u1.err.message);

    EmailStorageResult u2 = db.UpdateEmail(
        &db, (EmailRecord){.id = 0x42, .subject = "Ghost", .body = "Nobody home."});
    assert(u2.tag == EMAIL_STORAGE_ERR && "UPDATE 0x42 must fail — id never created");
    (void)u2;

    /* DELETE */
    EmailStorageResult d1 = db.DeleteEmail(&db, carol_id);
    assert(d1.tag == EMAIL_STORAGE_OK && "DELETE must succeed");
    (void)d1;

    EmailStorageResult d2 = db.DeleteEmail(&db, carol_id);
    assert(d2.tag == EMAIL_STORAGE_ERR && "DELETE twice must fail");
    (void)d2;

    EmailStorageResult gone = db.ReadEmail(&db, carol_id);
    assert(gone.tag == EMAIL_STORAGE_ERR && "READ after DELETE must fail");
    (void)gone;

    return 0;
}
