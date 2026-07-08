/*
    2026JUL06       (c) dbj@dbj.org

    gcc -std=c2x -Wall -Wswitch -Werror -I ../third_party/tau -o dbj_email_crud dbj_email_crud.c

    See dbj_discriminated_union.md for the design this implements.
    Single header (STB) style: dbj_email_record.h, dbj_email_storage_result.h
    and dbj_email_storage.h are declaration-only unless their
    *_IMPLEMENTATION macro is defined first, as done below.

    Assertions are now tau TEST cases (see third_party/tau) instead of
    bare assert(): REQUIRE_* aborts the current test on failure (same
    as assert did), CHECK_* records a failure and keeps going. tau's
    TAU_MAIN() replaces main() and prints a pass/fail summary instead
    of a silent abort.

    The CRUD verbs below build on each other (carol_id from CREATE
    feeds READ/DELETE, bob_id feeds UPDATE) against the one storage
    singleton — see dbj_email_storage.h — so this stays one test
    exercising the full lifecycle in order, rather than independent
    TEST cases that would need artificial ordering guarantees.
*/
#include <stdint.h>
#include <stdio.h>

#include "dbj_email_record.h"

#define DBJ_EMAIL_STORAGE_RESULT_IMPLEMENTATION
#include "dbj_email_storage_result.h"

#define DBJ_EMAIL_STORAGE_IMPLEMENTATION
#include "dbj_email_storage.h"

#include "../toplevel/simple_log.h"

#include <tau/tau.h>
TAU_MAIN()

TEST(EmailStorage, crud_flow) {
    EmailStorage* db = email_storage_instance();

    /* CREATE */
    EmailStorageResult c1 = db->CreateEmail(
        (EmailRecord){.to = "bob@example.com", .from = "alice@example.com",
                       .subject = "Q1 report", .body = "Please find attached..."});
    REQUIRE_TRUE(c1.tag == EMAIL_STORAGE_OK, "CREATE bob failed");
    EmailId bob_id = c1.ok.record.record_id;

    EmailStorageResult c2 = db->CreateEmail(
        (EmailRecord){.to = "carol@example.com", .from = "alice@example.com",
                       .subject = "Meeting notes", .body = "As discussed..."});
    REQUIRE_TRUE(c2.tag == EMAIL_STORAGE_OK, "CREATE carol failed");
    EmailId carol_id = c2.ok.record.record_id;

    /* READ */
    EmailStorageResult r1 = db->ReadEmail(carol_id);
    if (r1.tag == EMAIL_STORAGE_OK)
        SIMPLE_LOG("READ  id=%zu subject=%s", r1.ok.record.record_id, r1.ok.record.subject);
    else
        SIMPLE_LOG("READ  id=%zu failed — %s", carol_id, r1.err.message);
    CHECK_TRUE(r1.tag == EMAIL_STORAGE_OK, "READ carol failed");

    EmailStorageResult r2 = db->ReadEmail(0x42);
    REQUIRE_TRUE(r2.tag == EMAIL_STORAGE_ERR, "READ 0x42 must fail — id never created");

    /* UPDATE */
    EmailStorageResult u1 = db->UpdateEmail(
        (EmailRecord){.record_id = bob_id, .to = "bob@example.com",
                       .from = "alice@example.com",
                       .subject = "Q1 report (final)", .body = "Revised."});
    if (u1.tag == EMAIL_STORAGE_OK)
        SIMPLE_LOG("UPDATE id=%zu subject=%s", u1.ok.record.record_id, u1.ok.record.subject);
    else
        SIMPLE_LOG("UPDATE id=%zu failed — %s", bob_id, u1.err.message);
    CHECK_TRUE(u1.tag == EMAIL_STORAGE_OK, "UPDATE bob failed");

    EmailStorageResult u2 = db->UpdateEmail(
        (EmailRecord){.record_id = 0x42, .subject = "Ghost", .body = "Nobody home."});
    REQUIRE_TRUE(u2.tag == EMAIL_STORAGE_ERR, "UPDATE 0x42 must fail — id never created");

    /* DELETE */
    EmailStorageResult d1 = db->DeleteEmail(carol_id);
    REQUIRE_TRUE(d1.tag == EMAIL_STORAGE_OK, "DELETE must succeed");

    EmailStorageResult d2 = db->DeleteEmail(carol_id);
    REQUIRE_TRUE(d2.tag == EMAIL_STORAGE_ERR, "DELETE twice must fail");

    EmailStorageResult gone = db->ReadEmail(carol_id);
    REQUIRE_TRUE(gone.tag == EMAIL_STORAGE_ERR, "READ after DELETE must fail");
}
