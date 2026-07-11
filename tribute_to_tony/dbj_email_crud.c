/*
    2026JUL08       (c) dbj@dbj.org

    gcc -std=c2x -Wall -Wswitch -Werror -I ../third_party/tau -o dbj_email_crud dbj_email_crud.c

    See dbj_discriminated_union.md for the design this implements.
    Single header (STB) style: dbj_email_record.h, dbj_email_storage_result.h
    and dbj_email_storage.h are declaration-only unless their
    *_IMPLEMENTATION macro is defined first, as done below.

    Config ini file path is the program's first command line argument,
    not hardcoded, e.g.:

        dbj_email_crud dbj_email_crud.ini

    Expected content:

        NUMBER_OF_EMAILS = 1024
        EMAIL_FROM_ADDR = ...
        EMAIL_FROM_SUBJECT = ...
        EMAIL_FROM_BODY = ...
        EMAIL_TO_ADDR = ...
        EMAIL_TO_SUBJECT = ...
        EMAIL_TO_BODY = ...

    tau's TAU_MAIN() generates a main() that treats every argument as
    one of its own --flags and aborts on anything else, so it cannot
    also receive our ini path. We use TAU_NO_MAIN() instead and write
    our own main() that takes argv[1] as the ini path, strips it out,
    and forwards the remaining argv (program name plus any further
    tau options) to tau_main() -- so tau's own --filter=, --list, etc.
    still work after the ini path.

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
#include <tau/tau_bench.h>
TAU_NO_MAIN()

/* ── config, loaded once from the ini file named on the command line ── */
/* MIN_EMAILS_STORAGE_LIMIT / MAX_EMAILS_STORAGE_LIMIT come from
   dbj_email_storage.h -- they are the storage module's own operational
   boundaries, not something this test file should define locally. */

static char g_ini_path[BUFSIZ] = {};

/* argv[0] is the program name, argv[1] is our ini path -- neither is a
   tau option, so both are stripped before argc/argv reach tau_main(),
   which otherwise treats every argument as one of its own --flags and
   aborts on the first one it does not recognize. tau_argv0_ still gets
   the real program name (tau_argv[0] below), just not our ini path.

   Copies argv[1] with a plain snprintf, not DBJ_SNPRINTF below: that
   macro's REQUIRE_TRUE expands to a bare `return;` on failure, which is
   invalid in a function returning int like this one. */
int main(const int argc, const char* const* const argv) {
    printf("Built: %s %s\n\n", __DATE__, __TIME__);

    if (argc < 2) {
        printf("Usage: %s <ini-file> [tau options]\n\n", argv[0]);
        printf("<ini-file> must exist and look like this:\n\n");
        printf("    NUMBER_OF_EMAILS = %d .. %d\n", MIN_EMAILS_STORAGE_LIMIT, MAX_EMAILS_STORAGE_LIMIT);
        printf("    EMAIL_FROM_ADDR = alice@example.com\n");
        printf("    EMAIL_FROM_SUBJECT = Q1 report\n");
        printf("    EMAIL_FROM_BODY = Please find attached...\n");
        printf("    EMAIL_TO_ADDR = bob@example.com\n");
        printf("    EMAIL_TO_SUBJECT = Meeting notes\n");
        printf("    EMAIL_TO_BODY = As discussed...\n\n");
        printf("Current email storage capacity: %d\n", EMAIL_STORAGE_CAPACITY);
        return 1;
    }
    int ini_path_ret_ = snprintf(g_ini_path, sizeof g_ini_path, "%s", argv[1]);
    if (ini_path_ret_ < 0 || (size_t)ini_path_ret_ >= sizeof g_ini_path) {
        printf("ini file path truncated or failed to copy: %s\n", argv[1]);
        return 1;
    }

    const char* tau_argv[argc - 1];
    tau_argv[0] = argv[0];
    for (int i = 2; i < argc; i++)
        tau_argv[i - 1] = argv[i];

    return tau_main(argc - 1, tau_argv);
}

typedef struct {
    int  number_of_emails;
    char from_addr[EMAIL_RECORD_FROM_SIZE];
    char from_subject[EMAIL_RECORD_SUBJECT_SIZE];
    char from_body[EMAIL_RECORD_BODY_SIZE];
    char to_addr[EMAIL_RECORD_TO_SIZE];
    char to_subject[EMAIL_RECORD_SUBJECT_SIZE];
    char to_body[EMAIL_RECORD_BODY_SIZE];
} EmailCrudConfig;

static EmailCrudConfig g_email_crud_config = {};

/* ── N ids created by create_n, reused by read_n/update_n/delete_n ── */

static EmailId g_ids[EMAIL_STORAGE_CAPACITY] = {};

/* snprintf that aborts the test if the write was truncated or failed --
   silent truncation here would silently corrupt CRUD data. The one place
   truncation is accepted by design is format_subject_with_seq, which
   keeps its own plain snprintf and explains why. Only usable inside a
   void-returning function: REQUIRE_TRUE expands to a bare `return;` on
   failure, so it cannot be used inside ini_config_handler below (which
   returns int) -- see DBJ_SNPRINTF_HANDLER_REQUIRE for that case. */
#define DBJ_SNPRINTF(dest, size, ...)                                       \
    do {                                                                            \
        int dbj_snprintf_ret_ = snprintf((dest), (size), __VA_ARGS__);              \
        REQUIRE_TRUE(dbj_snprintf_ret_ >= 0 && (size_t)dbj_snprintf_ret_ < (size),  \
                     "snprintf truncated or failed writing to " #dest);             \
    } while (0)

/* Same check for use inside ini_config_handler, which returns int (the
   inifile.h callback contract) and so cannot use REQUIRE_TRUE. Returning
   0 here reports through the same error_/optional_line_no_ path as any
   other malformed ini line -- see the REQUIRE_TRUE checks on ini_result
   in load_and_validate_config. */
#define DBJ_SNPRINTF_HANDLER_REQUIRE(dest, size, ...)                    \
    do {                                                                 \
        int dbj_snprintf_ret_ = snprintf((dest), (size), __VA_ARGS__);   \
        if (dbj_snprintf_ret_ < 0 || (size_t)dbj_snprintf_ret_ >= (size)) \
            return 0;                                                   \
    } while (0)

static int ini_config_handler(void* user, const char section[static MAX_SECTION],
                               const char name[static MAX_NAME],
                               const char value[static INIFILE_MAX_LINE]) {
    (void)section;
    EmailCrudConfig* cfg = (EmailCrudConfig*)user;

    if (strcmp(name, "NUMBER_OF_EMAILS") == 0)
        cfg->number_of_emails = atoi(value);
    else if (strcmp(name, "EMAIL_FROM_ADDR") == 0)
        DBJ_SNPRINTF_HANDLER_REQUIRE(cfg->from_addr, sizeof cfg->from_addr, "%s", value);
    else if (strcmp(name, "EMAIL_FROM_SUBJECT") == 0)
        DBJ_SNPRINTF_HANDLER_REQUIRE(cfg->from_subject, sizeof cfg->from_subject, "%s", value);
    else if (strcmp(name, "EMAIL_FROM_BODY") == 0)
        DBJ_SNPRINTF_HANDLER_REQUIRE(cfg->from_body, sizeof cfg->from_body, "%s", value);
    else if (strcmp(name, "EMAIL_TO_ADDR") == 0)
        DBJ_SNPRINTF_HANDLER_REQUIRE(cfg->to_addr, sizeof cfg->to_addr, "%s", value);
    else if (strcmp(name, "EMAIL_TO_SUBJECT") == 0)
        DBJ_SNPRINTF_HANDLER_REQUIRE(cfg->to_subject, sizeof cfg->to_subject, "%s", value);
    else if (strcmp(name, "EMAIL_TO_BODY") == 0)
        DBJ_SNPRINTF_HANDLER_REQUIRE(cfg->to_body, sizeof cfg->to_body, "%s", value);

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

/* ── one phase per CRUD verb, kept as plain helpers (not TEST cases) so
   they can share g_ids across calls -- see the file header comment on
   why crud_n_flow is one TEST, not four. Each reaches g_email_crud_config/g_ids/
   storage_instance() directly rather than taking them as params: there
   is exactly one caller (the TEST below) and those three are already
   file-scope singletons, so threading them through as parameters would
   just be repeating the same three arguments at every call site.
   number_of_emails is the only value that is genuinely call-specific. ── */

/* create_email_storage_instance() re-wires the same function pointers
   every time it is called, so calling it once per _n function (as
   create_n/read_n/update_n/delete_n used to) is wasted, misleading
   work. storage_instance() caches the pointer in a function-local
   static, initialized on first call -- the standard C idiom for
   exactly-once (ISO C requires static-local initializers to be
   constant expressions, so the caching has to happen in the body, not
   in the static's own initializer). */
static EmailStorage* storage_instance(void) {
    static EmailStorage* singleton_ = nullptr;
    if (!singleton_)
        singleton_ = create_email_storage_instance();
// function call is not allowed in a constant expression C/C++(59)
// static EmailStorage* singleton_ = create_email_storage_instance();
        return singleton_;
}

static void load_and_validate_config(const char* ini_path, EmailCrudConfig* cfg) {
    REQUIRE_TRUE(ini_path != nullptr, "no ini file given on the command line");

    Inifile_result ini_result = ini_parse(ini_path, ini_config_handler, cfg);
    REQUIRE_TRUE(ini_result.error_ != ENOENT,
                 "Ini file: %s not found -- pass an existing ini file as the "
                 "first command line argument, see dbj_email_crud.c for its format",
                 ini_path);
    REQUIRE_TRUE(ini_result.error_ == 0 && ini_result.optional_line_no_ == 0,
                 "Ini file: %s failed to load (error_=%d, line=%d)",
                 ini_path, ini_result.error_, ini_result.optional_line_no_);

    REQUIRE_TRUE(cfg->number_of_emails >= MIN_EMAILS_STORAGE_LIMIT,
                 "Ini file: %s , NUMBER_OF_EMAILS %d is smaller than MIN_EMAILS_STORAGE_LIMIT %d",
                 ini_path, cfg->number_of_emails, MIN_EMAILS_STORAGE_LIMIT);
    REQUIRE_TRUE(cfg->number_of_emails <= MAX_EMAILS_STORAGE_LIMIT,
                 "Ini file: %s , NUMBER_OF_EMAILS %d is larger than EMAIL_STORAGE_CAPACITY %d",
                 ini_path, cfg->number_of_emails, EMAIL_STORAGE_CAPACITY);

    printf("Ini file: %s\n", ini_path);
    printf("    NUMBER_OF_EMAILS = %d\n", cfg->number_of_emails);
    printf("    EMAIL_FROM_ADDR = %s\n", cfg->from_addr);
    printf("    EMAIL_FROM_SUBJECT = %s\n", cfg->from_subject);
    printf("    EMAIL_FROM_BODY = %s\n", cfg->from_body);
    printf("    EMAIL_TO_ADDR = %s\n", cfg->to_addr);
    printf("    EMAIL_TO_SUBJECT = %s\n", cfg->to_subject);
    printf("    EMAIL_TO_BODY = %s\n", cfg->to_body);
}

static void create_n(int number_of_emails) {
    EmailStorage* db = storage_instance();
    EmailRecord   record = {};
    for (int i = 0; i < number_of_emails; i++) {
        DBJ_SNPRINTF(record.from, sizeof record.from, "%s", g_email_crud_config.from_addr);
        DBJ_SNPRINTF(record.to, sizeof record.to, "%s", g_email_crud_config.to_addr);
        format_subject_with_seq(record.subject, sizeof record.subject,
                                 g_email_crud_config.from_subject, i);
        DBJ_SNPRINTF(record.body, sizeof record.body, "%s", g_email_crud_config.from_body);

        EmailStorageResult r = db->CreateEmail(record);
        REQUIRE_TRUE(r.tag == EMAIL_STORAGE_OK, "CREATE failed mid-sequence");
        g_ids[i] = r.ok.record.record_id;
    }
}

static void read_n(int number_of_emails) {
    EmailStorage* db = storage_instance();
    for (int email_id = 0; email_id < number_of_emails; email_id++) {
        EmailStorageResult r = db->ReadEmail(  email_id + 1 /*g_ids[i]*/ );
        CHECK_TRUE(r.tag == EMAIL_STORAGE_OK, "READ failed mid-sequence");
    }
}

static void update_n(int number_of_emails) {
    EmailStorage* db = storage_instance();
    EmailRecord   record = {};
    for (int i = 0; i < number_of_emails; i++) {
        record.record_id = g_ids[i];
        DBJ_SNPRINTF(record.from, sizeof record.from, "%s", g_email_crud_config.from_addr);
        DBJ_SNPRINTF(record.to, sizeof record.to, "%s", g_email_crud_config.to_addr);
        format_subject_with_seq(record.subject, sizeof record.subject,
                                 g_email_crud_config.to_subject, i);
        DBJ_SNPRINTF(record.body, sizeof record.body, "%s", g_email_crud_config.to_body);

        EmailStorageResult r = db->UpdateEmail(record);
        CHECK_TRUE(r.tag == EMAIL_STORAGE_OK, "UPDATE failed mid-sequence");
    }
}

static void delete_n(int number_of_emails) {
    EmailStorage* db = storage_instance();
    for (int i = 0; i < number_of_emails; i++) {
        EmailStorageResult r = db->DeleteEmail(g_ids[i]);
        CHECK_TRUE(r.tag == EMAIL_STORAGE_OK, "DELETE failed mid-sequence");
    }
}

#define DBJ_TEXT_LINE SIMPLE_LOG("---------------------------------------------------------")

/* Runs one CRUD phase, logging start/finish and elapsed time around it.
   `timer` is a bare identifier (token-pasted by DBJ_TAU_START_TIMER/
   DBJ_TAU_ELAPSED and stringized here for both the phase name and the
   finish-line verb, e.g. "create" -- present tense, not "created").
   `n_function_name` is the phase's worker function, e.g. create_n. One
   place owns the log/timer format instead of one copy per phase. */
#define RUN_PHASE(timer, count, n_function_name)                            \
    do {                                                                    \
        DBJ_TEXT_LINE;                                                      \
        SIMPLE_LOG(#timer "_n: starting");                                  \
        DBJ_TAU_START_TIMER(timer);                                         \
        n_function_name(count);                                            \
        SIMPLE_LOG(#timer "_n: finished, " #timer " %d emails, elapsed %.2fms", \
                   count, DBJ_TAU_ELAPSED(timer) / 1e6);                    \
    } while (0)

TEST(DBJ_EMAIL_CRUD, TEST) {
    DBJ_TEXT_LINE;
    load_and_validate_config(g_ini_path, &g_email_crud_config);

    int number_of_emails = g_email_crud_config.number_of_emails;

    RUN_PHASE(create, number_of_emails, create_n);
    RUN_PHASE(read,   number_of_emails, read_n);
    RUN_PHASE(update, number_of_emails, update_n);
    RUN_PHASE(delete, number_of_emails, delete_n);
}
