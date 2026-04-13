/*
 * A tribute to C.A.R. Hoare, "Record Handling" (1966).
 *
 * Hoare's paper predates Simula 67, Smalltalk, and the entire OOP
 * vocabulary. He was not thinking in terms of objects sending messages
 * to each other. He was thinking about data with a type tag and code
 * that switches on that tag.
 *
 * What we have here is exactly that:
 *
 *   EmailCmd       — data with a type tag
 *   email_dispatch — code that switches on the tag
 *   impl_*(s, p)   — function that takes storage and params, nothing else
 *
 * No object. No `this`. No method lookup. No vtable.
 * Just a record, a tag, and a switch.
 *
 * The irony: Alan Kay also called his model "message passing" — but
 * Kay's dispatch is implicit, buried in the vtable, hidden behind the
 * dot operator. The receiver decides what to do; the caller cannot see
 * the mechanism.
 *
 * Hoare's version is explicit. The tag is visible data. The switch is
 * visible code. The compiler checks exhaustion via -Wswitch. Nothing
 * is hidden.
 *
 *   Kay  : object.method(args)  — receiver owns dispatch, caller is blind
 *   Hoare: dispatch(cmd)        — dispatch is explicit, tag is inspectable
 *
 * Simula read Hoare's paper, kept the subclass idea, and dropped
 * `inspect` — the keyword that would have given exhaustive switching on
 * the tag. That single omission is what Muratori calls the 35-year
 * mistake. C++ inherited Simula. Java inherited C++.
 *
 * What survived in the mainstream was Kay's model. What we have here
 * is closer to what Hoare actually wrote in 1966 — and closer to what
 * Rust's enum + match restored 58 years later.
 *
 * Implementation:
 *   EmailCmdParams : discriminated union over CRUD command parameters
 *   EmailCmd       : command envelope — CmdTag + EmailCmdParams
 *   Result         : discriminated union over OK / ERR
 *   EmailStorage   : flat array of EmailRecord, 0xFF slots, pure data
 *
 * C23: typed enums, designated initialisers, constexpr, auto,
 *      [[nodiscard]], nullptr, __VA_OPT__.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* ── logging ───────────────────────────────────────────────────── */

#define my_LOG(fmt, ...) \
    fprintf(stdout, "[%6d] " fmt "\n", __LINE__ __VA_OPT__(,) __VA_ARGS__)

#define UNWRAP(r_, id_var_)                                             \
    do {                                                                \
        if ((r_).tag == ERR) {                                          \
            my_LOG("UNWRAP failed — %s", (r_).err.msg);                \
            goto unwrap_fail;                                           \
        }                                                               \
        (id_var_) = (r_).ok.id;                                        \
    } while (0)

/* ── record ────────────────────────────────────────────────────── */

typedef uint8_t EmailId;   /* 0x00 = empty slot sentinel */

typedef struct {
    EmailId id;
    char    to[64];
    char    from[64];
    char    subject[128];
    char    body[512];
} EmailRecord;

/* ── result (discriminated union #1) ──────────────────────────── */

typedef enum : uint8_t { OK, ERR } ResultTag;

typedef struct {
    ResultTag tag;
    union {
        struct { EmailId id;    } ok;
        struct { char msg[128]; } err;
    };
} Result;

static inline Result result_ok(EmailId id) {
    return (Result){ .tag = OK, .ok = { id } };
}
static inline Result result_err(const char *msg) {
    Result r = { .tag = ERR };
    strncpy(r.err.msg, msg, sizeof r.err.msg - 1);
    return r;
}

/* ── command (discriminated union #2) ─────────────────────────── */
/*
 * Hoare's insight: a record with a type tag plus exhaustive switch.
 * All four CRUD variants share one memory region.
 * The compiler enforces exhaustion via -Wswitch.
 */

typedef enum : uint8_t {
    CMD_CREATE,
    CMD_READ,
    CMD_UPDATE,
    CMD_DELETE,
} CmdTag;

typedef struct {
    char to[64];
    char from[64];
    char subject[128];
    char body[512];
} CmdCreateParams;

typedef struct { EmailId id;                                    } CmdReadParams;
typedef struct { EmailId id; char subject[128]; char body[512]; } CmdUpdateParams;
typedef struct { EmailId id;                                    } CmdDeleteParams;

typedef union {
    CmdCreateParams create;
    CmdReadParams   read;
    CmdUpdateParams update;
    CmdDeleteParams del;
} EmailCmdParams;

typedef struct {
    CmdTag       tag;
    EmailCmdParams params;
} EmailCmd;

/* ── storage ───────────────────────────────────────────────────── */

constexpr uint8_t CAPACITY = 0xFF;

typedef struct {
    EmailRecord storage[CAPACITY];
} EmailStorage;

/* ── helpers ───────────────────────────────────────────────────── */

static EmailRecord *find_slot(EmailStorage *s, EmailId id) {
    for (uint8_t i = 0; i < CAPACITY; ++i)
        if (s->storage[i].id == id) return &s->storage[i];
    return nullptr;
}

static EmailRecord *find_empty(EmailStorage *s) {
    for (uint8_t i = 0; i < CAPACITY; ++i)
        if (s->storage[i].id == 0x00) return &s->storage[i];
    return nullptr;
}

static EmailId next_id = 0x01;

/* ── CRUD implementations ──────────────────────────────────────── */

static Result impl_create(EmailStorage *s, const CmdCreateParams *p)
{
    EmailRecord *slot = find_empty(s);
    if (!slot) {
        my_LOG("CREATE failed — storage full");
        return result_err("storage full");
    }
    *slot = (EmailRecord){ .id = next_id++ };
    strncpy(slot->to,      p->to,      sizeof slot->to      - 1);
    strncpy(slot->from,    p->from,    sizeof slot->from     - 1);
    strncpy(slot->subject, p->subject, sizeof slot->subject  - 1);
    strncpy(slot->body,    p->body,    sizeof slot->body     - 1);
    my_LOG("CREATE id=0x%02X to=%s subject=%s", slot->id, slot->to, slot->subject);
    return result_ok(slot->id);
}

[[nodiscard]]
static Result impl_read(const EmailStorage *s, const CmdReadParams *p,
                        EmailRecord *out)
{
    for (uint8_t i = 0; i < CAPACITY; ++i) {
        if (s->storage[i].id == p->id) {
            *out = s->storage[i];
            my_LOG("READ  id=0x%02X to=%s subject=%s",
                   p->id, out->to, out->subject);
            return result_ok(p->id);
        }
    }
    my_LOG("READ  id=0x%02X — not found", p->id);
    return result_err("not found");
}

static Result impl_update(EmailStorage *s, const CmdUpdateParams *p)
{
    EmailRecord *r = find_slot(s, p->id);
    if (!r) {
        my_LOG("UPDATE id=0x%02X — not found", p->id);
        return result_err("not found");
    }
    strncpy(r->subject, p->subject, sizeof r->subject - 1);
    strncpy(r->body,    p->body,    sizeof r->body    - 1);
    my_LOG("UPDATE id=0x%02X subject=%s", p->id, r->subject);
    return result_ok(p->id);
}

static Result impl_delete(EmailStorage *s, const CmdDeleteParams *p)
{
    EmailRecord *r = find_slot(s, p->id);
    if (!r) {
        my_LOG("DELETE id=0x%02X — not found", p->id);
        return result_err("not found");
    }
    my_LOG("DELETE id=0x%02X to=%s", p->id, r->to);
    *r = (EmailRecord){0};
    return result_ok(p->id);
}

/* ── single dispatch — the Hoare switch ───────────────────────── */

static Result email_dispatch(EmailStorage *s, const EmailCmd *cmd,
                             EmailRecord *out)
{
    switch (cmd->tag) {
        case CMD_CREATE: return impl_create(s, &cmd->params.create);
        case CMD_READ:   return impl_read(s,   &cmd->params.read, out);
        case CMD_UPDATE: return impl_update(s, &cmd->params.update);
        case CMD_DELETE: return impl_delete(s, &cmd->params.del);
    }
    __builtin_unreachable();
}

/* ── main ──────────────────────────────────────────────────────── */

int main(void) {
    my_LOG("=== my tribute to C.A.R. HOARE ===");
    my_LOG("===  20260319    dbj@dbj.org   ===");

    EmailStorage db  = { .storage = {{0}} };
    EmailRecord  rec = {0};
    EmailId id1 = 0, id2 = 0, id3 = 0;

    /* CREATE — build command, dispatch, unwrap the returned id */
    UNWRAP(email_dispatch(&db,
        &(EmailCmd){ .tag = CMD_CREATE, .params.create = {
            .to = "bob@example.com", .from = "alice@example.com",
            .subject = "Q1 report",  .body  = "Please find attached...",
        }}, &rec), id1);

    UNWRAP(email_dispatch(&db,
        &(EmailCmd){ .tag = CMD_CREATE, .params.create = {
            .to = "carol@example.com", .from = "alice@example.com",
            .subject = "Meeting notes", .body = "As discussed...",
        }}, &rec), id2);

    UNWRAP(email_dispatch(&db,
        &(EmailCmd){ .tag = CMD_CREATE, .params.create = {
            .to = "dave@example.com", .from = "bob@example.com",
            .subject = "Re: Q1 report", .body = "Looks good.",
        }}, &rec), id3);
    (void)id3;

    /* READ — happy path */
    email_dispatch(&db,
        &(EmailCmd){ .tag = CMD_READ, .params.read = { id2 } }, &rec);

    /* READ — sad path */
    email_dispatch(&db,
        &(EmailCmd){ .tag = CMD_READ, .params.read = { 0x42 } }, &rec);

    /* UPDATE — happy and sad */
    email_dispatch(&db,
        &(EmailCmd){ .tag = CMD_UPDATE, .params.update = {
            .id = id1, .subject = "Q1 report (final)", .body = "Revised.",
        }}, &rec);
    email_dispatch(&db,
        &(EmailCmd){ .tag = CMD_UPDATE, .params.update = {
            .id = 0x42, .subject = "Ghost", .body = "Nobody home.",
        }}, &rec);

    /* DELETE — happy then sad (same id twice) */
    email_dispatch(&db,
        &(EmailCmd){ .tag = CMD_DELETE, .params.del = { id2 } }, &rec);
    email_dispatch(&db,
        &(EmailCmd){ .tag = CMD_DELETE, .params.del = { id2 } }, &rec);

    /* READ after delete */
    email_dispatch(&db,
        &(EmailCmd){ .tag = CMD_READ, .params.read = { id2 } }, &rec);

    /* dump surviving slots */
    my_LOG("--- storage dump ---");
    for (uint8_t i = 0; i < CAPACITY; ++i) {
        const EmailRecord *r = &db.storage[i];
        if (r->id != 0x00)
            my_LOG("  slot[%03u] id=0x%02X to=%-22s subject=%s",
                   i, r->id, r->to, r->subject);
    }

    return 0;

unwrap_fail:
    my_LOG("aborted — unrecoverable error in CRUD chain");
    return 1;
}

/*
 * Build:
 *   gcc -std=c2x -Wall -Wswitch -Werror -o ai_tribute_to_tony ai_tribute_to_tony.c
 *
 * Discriminated unions in play:
 *   EmailCmd — dispatches CRUD; -Wswitch catches a missing case at compile time
 *   Result   — carries OK id or ERR message; UNWRAP guards the ok arm
 *
 * Layout:
 *   EmailRecord  = 1 + 64 + 64 + 128 + 512 = 769 bytes
 *   EmailStorage = 769 * 255               ≈ 196 KB — stack or BSS
 */