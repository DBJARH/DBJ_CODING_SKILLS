/*
    2026JUL08       (c) dbj@dbj.org

    gcc -std=c2x -Wall -Wswitch -Werror -I ../third_party/tau -o dbj_email_crud dbj_email_crud.c

    See dbj_discriminated_union.md for the design this implements.
    Single header (STB) style: dbj_email_record.h, dbj_email_storage_result.h
    and dbj_email_storage.h are declaration-only unless their
    *_IMPLEMENTATION macro is defined first, as done below.

    Config is read from dbj_email_crud.ini (same name as the executable),
    expected next to it at run time:

        NUMBER_OF_EMAILS = 1024
        EMAIL_FROM_ADDR = ...
        EMAIL_FROM_SUBJECT = ...
        EMAIL_FROM_BODY = ...
        EMAIL_TO_ADDR = ...
        EMAIL_TO_SUBJECT = ...
        EMAIL_TO_BODY = ...

    All NUMBER_OF_EMAILS emails share the same from/to/subject/body out
    of the ini, except the subject gets " #<sequence number>" appended
    so each of the N emails is distinguishable in storage/logs.

    NUMBER_OF_EMAILS must not exceed EMAIL_STORAGE_CAPACITY (a compile
    time setting, see dbj_email_storage.h) -- create_n_emails checks
    this once, before creating anything, and aborts the whole run via
    REQUIRE_TRUE if it does not hold.

    The four CRUD verbs are four phases of one TEST case, not four
    separate TEST cases: tau does not run TEST cases in declaration
    order (verified -- it runs them in reverse), so separate CREATE/
    READ/UPDATE/DELETE TEST cases could not reliably share the N ids
    CREATE produces. One TEST, run against the one storage singleton
    (see dbj_email_storage.h), avoids relying on any cross-test
    ordering guarantee tau does not provide.

    Per-email logging would be N lines per phase, which is not useful
    signal at N=1024, so each phase logs only once at its start and
    once at its end, not per CRUD call.
*/
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dbj_email_record.h"

#define DBJ_EMAIL_STORAGE_RESULT_IMPLEMENTATION
#include "dbj_email_storage_result.h"

#define DBJ_EMAIL_STORAGE_IMPLEMENTATION
#include "dbj_email_storage.h"

#include "../toplevel/simple_log.h"

#define INIFILE_IMPLEMENTATION
#include "../third_party/inifile/inifile.h"

#include <tau/tau.h>
TAU_MAIN()

/* ── config, loaded once from dbj_email_crud.ini ─────────────────── */
/* MIN_EMAILS_STORAGE_LIMIT / MAX_EMAILS_STORAGE_LIMIT come from
   dbj_email_storage.h -- they are the storage module's own operational
   boundaries, not something this test file should define locally. */

typedef struct {
    int  number_of_emails;
    char from_addr[EMAIL_RECORD_FROM_SIZE];
    char from_subject[EMAIL_RECORD_SUBJECT_SIZE];
    char from_body[EMAIL_RECORD_BODY_SIZE];
    char to_addr[EMAIL_RECORD_TO_SIZE];
    char to_subject[EMAIL_RECORD_SUBJECT_SIZE];
    char to_body[EMAIL_RECORD_BODY_SIZE];
} EmailCrudConfig;

static EmailCrudConfig g_config = {};

/* ── N ids created by create_n, reused by read_n/update_n/delete_n ── */

static EmailId g_ids[EMAIL_STORAGE_CAPACITY] = {};

static int ini_config_handler(void* user, const char section[static MAX_SECTION],
                               const char name[static MAX_NAME],
                               const char value[static INIFILE_MAX_LINE]) {
    (void)section;
    EmailCrudConfig* cfg = (EmailCrudConfig*)user;

    if (strcmp(name, "NUMBER_OF_EMAILS") == 0)
        cfg->number_of_emails = atoi(value);
    else if (strcmp(name, "EMAIL_FROM_ADDR") == 0)
        snprintf(cfg->from_addr, sizeof cfg->from_addr, "%s", value);
    else if (strcmp(name, "EMAIL_FROM_SUBJECT") == 0)
        snprintf(cfg->from_subject, sizeof cfg->from_subject, "%s", value);
    else if (strcmp(name, "EMAIL_FROM_BODY") == 0)
        snprintf(cfg->from_body, sizeof cfg->from_body, "%s", value);
    else if (strcmp(name, "EMAIL_TO_ADDR") == 0)
        snprintf(cfg->to_addr, sizeof cfg->to_addr, "%s", value);
    else if (strcmp(name, "EMAIL_TO_SUBJECT") == 0)
        snprintf(cfg->to_subject, sizeof cfg->to_subject, "%s", value);
    else if (strcmp(name, "EMAIL_TO_BODY") == 0)
        snprintf(cfg->to_body, sizeof cfg->to_body, "%s", value);

    return 1; /* unknown keys are ignored, not an error */
}

/*
 * base can be up to EMAIL_RECORD_SUBJECT_SIZE bytes on its own (it comes
 * straight from the ini), so appending " #<seq>" can truncate -- that is
 * fine here, subject is informational only, so the truncation warning is
 * suppressed locally instead of working around it with a bigger buffer.
 */
static void format_subject_with_seq(char dest[static EMAIL_RECORD_SUBJECT_SIZE],
                                     size_t dest_size,
                                     const char base[static EMAIL_RECORD_SUBJECT_SIZE],
                                     int seq) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    snprintf(dest, dest_size, "%s #%d", base, seq);
#pragma GCC diagnostic pop
}

TEST(EmailStorage, crud_n_flow) {
    Inifile_result ini_result = ini_parse("dbj_email_crud.ini", ini_config_handler, &g_config);
    REQUIRE_TRUE(ini_result.error_ == 0 && ini_result.optional_line_no_ == 0,
                 "failed to load dbj_email_crud.ini");

    REQUIRE_TRUE(g_config.number_of_emails >= MIN_EMAILS_STORAGE_LIMIT,
                 "Ini file: %s , NUMBER_OF_EMAILS %d is smaller than MIN_EMAILS_STORAGE_LIMIT %d",
                 "dbj_email_crud.ini", g_config.number_of_emails, MIN_EMAILS_STORAGE_LIMIT);
    REQUIRE_TRUE(g_config.number_of_emails <= MAX_EMAILS_STORAGE_LIMIT,
                 "Ini file: %s , NUMBER_OF_EMAILS %d is larger than EMAIL_STORAGE_CAPACITY %d",
                 "dbj_email_crud.ini", g_config.number_of_emails, EMAIL_STORAGE_CAPACITY);

    EmailStorage* db = email_storage_instance();
    int n = g_config.number_of_emails;

    /* CREATE */
    SIMPLE_LOG("create_n: starting");
    for (int i = 0; i < n; i++) {
        EmailRecord record = {};
        snprintf(record.from, sizeof record.from, "%s", g_config.from_addr);
        snprintf(record.to, sizeof record.to, "%s", g_config.to_addr);
        format_subject_with_seq(record.subject, sizeof record.subject,
                                 g_config.from_subject, i);
        snprintf(record.body, sizeof record.body, "%s", g_config.from_body);

        EmailStorageResult r = db->CreateEmail(record);
        REQUIRE_TRUE(r.tag == EMAIL_STORAGE_OK, "CREATE failed mid-sequence");
        g_ids[i] = r.ok.record.record_id;
    }
    SIMPLE_LOG("create_n: finished, created %d emails", n);

    /* READ */
    SIMPLE_LOG("read_n: starting");
    for (int i = 0; i < n; i++) {
        EmailStorageResult r = db->ReadEmail(g_ids[i]);
        CHECK_TRUE(r.tag == EMAIL_STORAGE_OK, "READ failed mid-sequence");
    }
    SIMPLE_LOG("read_n: finished, read %d emails", n);

    /* UPDATE */
    SIMPLE_LOG("update_n: starting");
    for (int i = 0; i < n; i++) {
        EmailRecord record = {};
        record.record_id = g_ids[i];
        snprintf(record.from, sizeof record.from, "%s", g_config.from_addr);
        snprintf(record.to, sizeof record.to, "%s", g_config.to_addr);
        format_subject_with_seq(record.subject, sizeof record.subject,
                                 g_config.to_subject, i);
        snprintf(record.body, sizeof record.body, "%s", g_config.to_body);

        EmailStorageResult r = db->UpdateEmail(record);
        CHECK_TRUE(r.tag == EMAIL_STORAGE_OK, "UPDATE failed mid-sequence");
    }
    SIMPLE_LOG("update_n: finished, updated %d emails", n);

    /* DELETE */
    SIMPLE_LOG("delete_n: starting");
    for (int i = 0; i < n; i++) {
        EmailStorageResult r = db->DeleteEmail(g_ids[i]);
        CHECK_TRUE(r.tag == EMAIL_STORAGE_OK, "DELETE failed mid-sequence");
    }
    SIMPLE_LOG("delete_n: finished, deleted %d emails", n);
}
