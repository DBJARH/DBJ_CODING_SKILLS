# Design to go with [dbj_discriminated_union.c](dbj_discriminated_union.c)


> **Caveat Emptor**: We are enjoying the metapresence of [DBJ Taxonomies](https://method.dbj.org/taxonomy_core.html). Thus we know where are we in the information space with these endeavor. Top category: **Implementation**. Capability: **Development**. In other word: We know what is this all about. And we can explain it to the reader.
>

**Table of Contents**
- [Design to go with dbj\_discriminated\_union.c](#design-to-go-with-dbj_discriminated_unionc)
- [Top-level logical design](#top-level-logical-design)
  - [Top level requirement: RQ01](#top-level-requirement-rq01)
  - [User / EmailStorage interaction](#user--emailstorage-interaction)
  - [**EmailRecord**](#emailrecord)
  - [EmailStorageResult](#emailstorageresult)
  - [EmailStorage](#emailstorage)
      - [**EmailStorage API**](#emailstorage-api)
      - [Notice on user required and assumed behavior](#notice-on-user-required-and-assumed-behavior)
    - [Current Email Storage API](#current-email-storage-api)
  - [Multi Threading](#multi-threading)



# Top-level logical design

## Top level requirement: [RQ01](top_level_requirements.md#rq01-email-crud-application)

## User / EmailStorage interaction

```mermaid
%%{init: {'theme':'base', 'themeVariables': {'background':'#ffffff'}}}%%
sequenceDiagram
    actor User as EmailStorageUser
    participant Storage as EmailStorage

    User->>Storage: CreateEmail(fields...)
    Storage-->>User: EmailStorageResult (EmailRecord | err)

    User->>Storage: ReadEmail(id)
    Storage-->>User: EmailStorageResult (EmailRecord | err)
```


## **EmailRecord**

- `EmailRecord` is the central, tagged type.
  - Storage is a logical array of `EmailRecord`s, plus a singly-linked
    free list threaded through the same array for slot reuse
    - We consider this a specialized storage for `EmailRecord`s
    - There is no table here, so no `ROWID` in the SQLite sense either
      — but the same distinction is worth naming explicitly, since it is
      exactly what trips people up: this storage gives every record
      **two** ids, with two different lifetimes
      - `EmailRecord.slot_id` — a plain 0-based array index, what CRUD
        actually keys every lookup on. No reserved "empty" id; every
        value, including `0`, can be a real record's `slot_id`
        - occupancy (is this slot live right now) is tracked
          separately, by storage's own `occupied[]` array — not by any
          special value of `slot_id`, so there is nothing for `slot_id`
          to avoid
        - a deleted slot is pushed onto the free list and its index is
          reissued to the next `CreateEmail`, so **`slot_id`s are
          reused**: a `slot_id` is only a stable identity while its
          record is live, not a permanent one — after delete, that
          numeric id will come back attached to a different record once
          its slot is reused
        - this bounds capacity by *live* records, not by total creates
          ever made — churn (create/delete/create/...) does not burn
          through `EMAIL_STORAGE_CAPACITY`
      - `EmailRecord.unique_id` — assigned once on `CreateEmail`, from a
        counter that only ever increments, and never reused or looked
        up by. Not used by any CRUD verb; it exists purely so a
        record's identity is legible even after its `slot_id` has been
        reissued — read a record back, and its `unique_id` tells you
        whether you are looking at the same record you created or a
        different one that has since landed in the same slot
        - its own type, `UniqueId`, is a distinct typedef from `EmailId`
          — the two ids answer different questions (which slot, vs which
          record), so sharing one type made that distinction easy to
          miss at a glance

```mermaid
graph LR
    subgraph EmailStorage
        R1["records[0] slot_id=0 unique_id=7 (occupied)"]
        R2["records[1] slot_id=1 unique_id=8 (occupied)"]
        R3["records[2] (free)"]
        R4["records[3] (free)"]
    end
    free_list_head --> R4 -. next_free_slot .-> R3
```

**Synopsys**

```c
typedef U8TYPE EmailId;
typedef U8TYPE UniqueId;

typedef struct {
    EmailId  slot_id;   // reused after delete -- what CRUD keys on
    UniqueId unique_id; // monotonic, never reused, unused by CRUD
    char to[64];
    char from[64];
    char subject[128];
    char body[512];
} EmailRecord;

```


## EmailStorageResult

Standard return type is: `EmailStorageResult`. It is a tagged union, returning the `EmailRecord` or an error
```
EmailStorageResult.err
    char[512] : error location (file, function, line_number)
    char[512] : error message
```

**Synopsis**

```c
typedef enum : U8TYPE {
    EMAIL_STORAGE_OK,
    EMAIL_STORAGE_ERR,
} EmailStorageResultTag;

typedef struct EmailStorageResult EmailStorageResult;

struct EmailStorageResult {
    EmailStorageResultTag tag;
    union {
        struct {
            EmailStorageResult (*make)(EmailRecord record);
            EmailRecord record;
        } ok;
        struct {
            EmailStorageResult (*make)(const char* location, const char* message);
            char location[512]; /* file:function:line, e.g. via __FILE__ ":" __func__ ":" ... */
            char message[512];
        } err;
    };
};
```

**Future improvements**. Notice we say "improvements" not "extensions".

- user configurable size of `location` and `message` char arrays, on the `err` struct.
  - that also allows for using char array parameters with size hint
- both `location` and `message` in a json format
  - Discuss: why not just one json formatted `payload`?

## EmailStorage

    - Logically has associated CRUD verbs
      - That is the language it speaks
    - CRUD Verbs are logically executed inside a `EmailStorage` and accessible over its API (Interface)
      - CRUD verbs are dispatch, not data: each verb is a plain function
        - Data is passed in and out as function parameters nad return values
    - CRUD verbs are
      - CreateEmail
      - ReadEmail
      - UpdateEmail
      - DeleteEmail
    - Thus those are the core methods of the `EmailStorage` interface
    - **EmailStorageUser** knows the language of the `EmailStorage`

#### **EmailStorage API**

**EmailStorage API Synopsys 0**

```c
typedef struct EmailStorage EmailStorage;

struct EmailStorage {
    EmailRecord records[/* capacity */];

    EmailStorageResult (*CreateEmail)(EmailStorage* self, EmailRecord record);
    EmailStorageResult (*ReadEmail)(EmailStorage* self, EmailId id);
    EmailStorageResult (*UpdateEmail)(EmailStorage* self, EmailRecord record);
    EmailStorageResult (*DeleteEmail)(EmailStorage* self, EmailId id);
};
```

`CreateEmail`/`ReadEmail`/`UpdateEmail`/`DeleteEmail` are function
pointers carried on the `EmailStorage` instance itself — same "make"
pattern already used by `Result` and `EmailStorageResult` 

#### Notice on user required and assumed behavior

In order to use this API (Interface) User has to first obtain the instance to the storage. Plus to that is that user can potentially use multiple storages. Minus is that obtaining the storage or storages instances is undefined on the system level. For example. System architecture might conclude a single email storage is preferred but has to be externally controlled. Or the opposite. Many/several encapsulated storages. In any case that is out of the scope of this design.

Probably on of the good and feasible solutions is to encapsulate the storage instance obtaining behind a single function:

```c
   EmailStorage * email_storage_instance (/* reserver for future use */) ;
```

Using a simple pointer might be a bit dangerous but some other (of many) design might be a overkill for this context.

in any case it is not a good practice to leave it to the users how will be the storage instance obtained. So we will encapsulate that behind a storage interface that is "in process" in relative to the user location.

### Current Email Storage API

**EmailStorage API Synopsys 1**

Storage is a fixed `records[EMAIL_STORAGE_CAPACITY]` array, exactly as
in Synopsys 0, plus a singly-linked free list threaded through it via
`next_free_slot` — so CRUD stays O(1) (direct index by `slot_id`),
with no heap allocation. `slot_id` is a plain 0-based slot index, no
reserved "empty" value: occupancy is tracked independently by
`occupied[]`, so `slot_id` itself never needs to avoid a particular
value. A deleted slot's index goes onto `free_list_head` and is
reissued to the next `CreateEmail`, so churn (create/delete/create/...)
reuses slots instead of exhausting `high_water_mark` toward
`EMAIL_STORAGE_CAPACITY`. `live_count` is the number of records
currently in storage (up on create, down on delete) — distinct from
`high_water_mark`, the count of slots ever occupied at least once.

`EmailRecord.unique_id` (see EmailRecord above) comes from a
`next_unique_id()` function, not a struct field — its counter is
unrelated to slot reuse, so it does not belong next to the slot
bookkeeping (`free_list_head`/`high_water_mark`/`live_count`) on
`EmailStorage`. `CreateEmail` calls it once per record.

```c
typedef struct EmailStorage EmailStorage;

   // encapsulated behind Interface 
   EmailStorage * email_storage_instance (/* reserver for future use */) ;

struct EmailStorage {
    EmailRecord records[/* capacity */];

    bool   occupied[/* capacity */];       /* is this slot live right now */
    size_t next_free_slot[/* capacity */]; /* free-list links, by index */
    size_t free_list_head;                 /* or "empty" sentinel */
    size_t high_water_mark;                /* slots ever occupied */
    size_t live_count;                     /* records in storage now */

    EmailStorageResult (*CreateEmail)( EmailRecord record);
    EmailStorageResult (*ReadEmail)( EmailId id);
    EmailStorageResult (*UpdateEmail)( EmailRecord record);
    EmailStorageResult (*DeleteEmail)( EmailId id);
};
```

## Multi Threading

Currently the design and code do not work in presence of multiple threads. That will be relatively straightforward to solve. We will use a single light mutex to be reachable from MailStorage public API and lock on entry unlock on leaving pattern. For that to be simple and resilient we will use the mandated compiler (GCC 15+) `defer` statement.
