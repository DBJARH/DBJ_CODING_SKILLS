/*
    2026APR04       (c) dbj@dbj.org     human optimizes version of https://godbolt.org/z/sb7MTeGax
    
     gcc -std=c2x -Wall -Wswitch -Werror -o tribute_to_tony tribute_to_tony.c
*/
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* ── logging ───────────────────────────────────────────────────── */

#define my_LOG(fmt, ...) \
    fprintf(stdout, "[%6d] " fmt "\n", __LINE__ __VA_OPT__(, ) __VA_ARGS__)

#ifdef NDEBUG
#define my_DBG(fmt, ...) ((void)0)
#define DBG_LOG_RESULT(r_) ((void)0)
#define DBG_LOG_EMAIL_CMD(cmd_) ((void)0)
#else

#define my_DBG(fmt, ...)                              \
    fprintf(stdout, "[%6d] %s() " fmt "\n", __LINE__, \
            __func__ __VA_OPT__(, ) __VA_ARGS__)

#define DBG_LOG_RESULT(r_)                           \
    do {                                             \
        if ((r_).tag == OK)                          \
            my_DBG("result OK  id=%u", (r_).ok.id);  \
        else                                         \
            my_DBG("result ERR — %s", (r_).err.msg); \
    } while (0)

#define DBG_LOG_EMAIL_CMD(cmd_)                                              \
    do {                                                                     \
        switch ((cmd_)->tag) {                                               \
            case CMD_CREATE:                                                 \
                my_DBG("CMD CREATE to=%s from=%s subject=%s",                \
                       (cmd_)->params.create.to, (cmd_)->params.create.from, \
                       (cmd_)->params.create.subject);                       \
                break;                                                       \
            case CMD_READ:                                                   \
                my_DBG("CMD READ id=%u", (cmd_)->params.read.id);            \
                break;                                                       \
            case CMD_UPDATE:                                                 \
                my_DBG("CMD UPDATE id=%u subject=%s",                        \
                       (cmd_)->params.update.id,                             \
                       (cmd_)->params.update.subject);                       \
                break;                                                       \
            case CMD_DELETE:                                                 \
                my_DBG("CMD DELETE id=%u", (cmd_)->params.del.id);           \
                break;                                                       \
        }                                                                    \
    } while (0)
#endif

// typedef unsigned char U8TYPE ;
// due to Code intelisense limitations we do mthe following
#define U8TYPE unsigned char

/* ── result (discriminated union #1) ──────────────────────────── */

typedef U8TYPE RetVAL; /* value carried in the ok arm of Result */

typedef enum : U8TYPE {
    OK,
    ERR,
} ResultTag;

typedef struct {
    ResultTag tag;
    union {
        struct {
            RetVAL id;
        } ok;
        struct {
            char msg[128];
        } err;
    };
} Result;

// could be macro, but ...
static inline Result result_ok(RetVAL id) {
    return (Result){.tag = OK, .ok = {id}};
}
static inline Result result_err(const char* func, const char* msg) {
    assert(func && "result_err called with null func");
    assert(msg && "result_err called with null message");
    Result r = {.tag = ERR};
    snprintf(r.err.msg, sizeof r.err.msg, "%s — %s", func, msg);
    return r;
}

/* ── record ──────────────────────────────────────────────────────v */

typedef U8TYPE EmailId;

#define EMAIL_ID_NULL \
    ((EmailId)0x00) /* reserved — marks empty slot and null record */

typedef struct {
    EmailId id;
    char to[64];
    char from[64];
    char subject[128];
    char body[512];
} EmailRecord;

static const EmailRecord NULL_EMAIL_RECORD = {.id = EMAIL_ID_NULL};

/* ── command (discriminated union #2) ─────────────────────────── */
/*
 * Hoare's insight: a record with a type tag plus exhaustive switch.
 * All four CRUD variants share one memory region.
 * The compiler enforces exhaustion via -Wswitch.
 */

typedef enum : U8TYPE {
    CMD_CREATE,
    CMD_READ,
    CMD_UPDATE,
    CMD_DELETE,
} EmailCrudCmdTag;

typedef struct {
    EmailCrudCmdTag tag;
    union {
        struct {
            char to[64];
            char from[64];
            char subject[128];
            char body[512];
        } create;
        struct {
            EmailId id;
        } read;
        struct {
            EmailId id;
            char subject[128];
            char body[512];
        } update;
        struct {
            EmailId id;
        } del;
    } params;
} EmailCmd;

/* ── command constructors ──────────────────────────────────────── */

#define EMAIL_CREATE_CMD(to_, from_, sub_, body_) \
    &(EmailCmd){.tag = CMD_CREATE,                \
                .params.create = {(to_), (from_), (sub_), (body_)}}
#define EMAIL_READ(id_) &(EmailCmd){.tag = CMD_READ, .params.read = {(id_)}}
#define EMAIL_UPDATE(id_, sub_, body_) \
    &(EmailCmd){.tag = CMD_UPDATE, .params.update = {(id_), (sub_), (body_)}}
#define EMAIL_DELETE(id_) &(EmailCmd){.tag = CMD_DELETE, .params.del = {(id_)}}

/* ── storage ─────────────────────────────────────────────────────
  notice how is this storage completely coupled to the type it stores
*/

// constexpr U8TYPE CAPACITY = 0xFF;
// also due to VS Code *sense problem we do the bellow
#define CAPACITY 0xF

typedef struct {
    // note bellow how in helpers
    // we return adresses from this array slots
    EmailRecord storage[CAPACITY];
} EmailStorage;

/* ── storage helpers ─────────────────────────────────────────────── */

static EmailRecord* find_first_non_empty_slot(EmailStorage* s, EmailId id) {
    assert(id != EMAIL_ID_NULL &&
           "find_first_non_empty_slot called with null id");

    for (U8TYPE i = 0; i < CAPACITY; ++i)
        if (s->storage[i].id == id) return &s->storage[i];
    return nullptr;
}

static EmailRecord* find_empty(EmailStorage* s) {
    for (U8TYPE i = 0; i < CAPACITY; ++i)
        if (s->storage[i].id == EMAIL_ID_NULL) return &s->storage[i];
    return nullptr;
}

#ifdef STORAGE_DUMP_WILL_BE_NEEDED

static void storage_dump(const EmailStorage* s) {
    for (U8TYPE slot_idx = 0; slot_idx < CAPACITY; ++slot_idx) {
        const EmailRecord* r = &s->storage[slot_idx];
        if (r->id != EMAIL_ID_NULL)
            my_LOG(" slot[%03u] Emil id=[%-3u] to=%-22s subject=%s", slot_idx,
                   r->id, r->to, r->subject);
        else
            my_LOG(" slot[%03u] Null Email", slot_idx);
    }
}

#endif

static EmailId next_id = 0x01;

/* ── CRUD implementations ──────────────────────────────────────── */

static Result impl_create(EmailStorage* s, const EmailCmd* cmd) {
    EmailRecord* slot = find_empty(s);
    if (!slot) return result_err(__func__, "storage full");
    *slot = (EmailRecord){.id = next_id++};
    memcpy(slot->to, cmd->params.create.to, sizeof slot->to - 1);
    memcpy(slot->from, cmd->params.create.from, sizeof slot->from - 1);
    memcpy(slot->subject, cmd->params.create.subject, sizeof slot->subject - 1);
    memcpy(slot->body, cmd->params.create.body, sizeof slot->body - 1);
    return result_ok(slot->id);
}

static Result impl_read(const EmailStorage* s, const EmailCmd* cmd) {
    for (U8TYPE i = 0; i < CAPACITY; ++i)
        if (s->storage[i].id == cmd->params.read.id)
            return result_ok(cmd->params.read.id);
    return result_err(__func__, "not found");
}

static Result impl_update(EmailStorage* s, const EmailCmd* cmd) {
    EmailRecord* r = find_first_non_empty_slot(s, cmd->params.update.id);
    if (!r) return result_err(__func__, "not found");
    memcpy(r->subject, cmd->params.update.subject, sizeof r->subject - 1);
    memcpy(r->body, cmd->params.update.body, sizeof r->body - 1);
    return result_ok(cmd->params.update.id);
}

static Result impl_delete(EmailStorage* s, const EmailCmd* cmd) {
    EmailRecord* r = find_first_non_empty_slot(s, cmd->params.del.id);
    if (!r) return result_err(__func__, "not found");
    *r = (EmailRecord){.id = EMAIL_ID_NULL};  // implicit and deliberate!
    return result_ok(cmd->params.del.id);
}

/* ── single dispatch — the Hoare switch ───────────────────────── */

static EmailRecord email_dispatch(EmailStorage* s, const EmailCmd* cmd) {
    Result dispatch_result = {};
    switch (cmd->tag) {
        case CMD_CREATE:
            dispatch_result = impl_create(s, cmd);
            break;
        case CMD_READ:
            dispatch_result = impl_read(s, cmd);
            break;
        case CMD_UPDATE:
            dispatch_result = impl_update(s, cmd);
            break;
        case CMD_DELETE:
            dispatch_result = impl_delete(s, cmd);
            break;
    }

    DBG_LOG_EMAIL_CMD(cmd);
    if (dispatch_result.tag == ERR) return NULL_EMAIL_RECORD;

    if (cmd->tag == CMD_DELETE) return NULL_EMAIL_RECORD;

    EmailRecord* found = find_first_non_empty_slot(s, dispatch_result.ok.id);
    assert(found && "result_ok id not found in storage — storage corrupted");

    DBG_LOG_RESULT(dispatch_result);
    return *found;
}

/* ── main ──────────────────────────────────────────────────────── */

int main(void) {
    my_LOG("=== my tribute to C.A.R. HOARE ===");
    my_LOG("===  2026APR03  (c) dbj@dbj.org   ===");
#ifdef NDEBUG
    my_LOG("=== RELEASE build | %s %s ===", __DATE__, __TIME__);
#else
    my_LOG("=== DEBUG   build | %s %s ===", __DATE__, __TIME__);
#endif
#if defined(__clang__)
    my_LOG("=== compiler: Clang " __VERSION__ " ===");
#elif defined(__GNUC__)
    my_LOG("=== compiler: GCC " __VERSION__ " ===");
#else
    my_LOG("=== compiler: unknown ===");
#endif

    EmailStorage db = {.storage = {{0}}};

    {
        /* CREATE */
        EmailRecord e1 = email_dispatch(
            &db, EMAIL_CREATE_CMD("bob@example.com", "alice@example.com",
                                  "Q1 report", "Please find attached..."));
        assert(e1.id != EMAIL_ID_NULL && "CREATE bob failed");

        EmailRecord e2 = email_dispatch(
            &db, EMAIL_CREATE_CMD("carol@example.com", "alice@example.com",
                                  "Meeting notes", "As discussed..."));
        assert(e2.id != EMAIL_ID_NULL && "CREATE carol failed");

        EmailRecord e3 = email_dispatch(
            &db, EMAIL_CREATE_CMD("dave@example.com", "bob@example.com",
                                  "Re: Q1 report", "Looks good."));
        assert(e3.id != EMAIL_ID_NULL && "CREATE dave failed");

        /* READ */
        EmailRecord r1 = email_dispatch(&db, EMAIL_READ(e2.id));
        if (r1.id == EMAIL_ID_NULL) my_LOG("READ id=%u failed", e2.id);

        EmailRecord r2 = email_dispatch(&db, EMAIL_READ(0x42));
        assert(r2.id == EMAIL_ID_NULL &&
               "READ 0x42 must return null record — id never created");
        (void)r2;

        /* UPDATE */
        EmailRecord u1 = email_dispatch(
            &db, EMAIL_UPDATE(e1.id, "Q1 report (final)", "Revised."));
        if (u1.id == EMAIL_ID_NULL) my_LOG("UPDATE id=%u failed", e1.id);

        EmailRecord u2 =
            email_dispatch(&db, EMAIL_UPDATE(0x42, "Ghost", "Nobody home."));
        assert(u2.id == EMAIL_ID_NULL &&
               "UPDATE 0x42 must return null record — id never created");
        (void)u2;

        /* DELETE */
        EmailRecord d1 = email_dispatch(&db, EMAIL_DELETE(e2.id));
        assert(d1.id == EMAIL_ID_NULL && "DELETE must return null record");
        (void)d1;

        EmailRecord d2 = email_dispatch(&db, EMAIL_DELETE(e2.id));
        assert(d2.id == EMAIL_ID_NULL &&
               "DELETE twice must return null record");
        (void)d2;

        EmailRecord gone = email_dispatch(&db, EMAIL_READ(e2.id));
        assert(gone.id == EMAIL_ID_NULL &&
               "READ after DELETE must return null record");
        (void)gone;
    }

#ifdef STORAGE_DUMP_WILL_BE_NEEDED
    my_LOG("--- storage dump ---");
    storage_dump(&db);
#endif

    return 0;
}
/*
 * *****************************************************************************
 *
  *
 */