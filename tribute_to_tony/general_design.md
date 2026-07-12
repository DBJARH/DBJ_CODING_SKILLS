<h1> General Design Notes </h1>

<h1 style="font-size:10rem; font-weight:normal; font-family:georgia"> W.I.P. </h1>


> First time visitor can understand easily what is this all about. And we can explain it better. 
> 
> We are enjoying the metapresence of [DBJ Taxonomies](https://method.dbj.org/taxonomy_core.html). Thus we can communicate where are we in the information space with this doc. 
>

```
Category:       Implementation
Capability:     Development
```

**Table of Contents**
- [Top-level logical design](#top-level-logical-design)
  - [Top level requirement: RQ01](#top-level-requirement-rq01)
  - [User / EmailStorage interaction](#user--emailstorage-interaction)
  - [**EmailRecord**](#emailrecord)
  - [EmailStorageResult](#emailstorageresult)
  - [EmailStorage](#emailstorage)
    - [Free Slots concept](#free-slots-concept)
  - [Note on Multi Threading](#note-on-multi-threading)



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
**Notice on user required and assumed behavior**

In order to use this API (Interface) User has to first obtain the instance to the storage. Plus to that solution, is that user can potentially use multiple storages. Minus is that obtaining the storage or storages instances is yet undefined on the system level. For example. System architecture might conclude a single email storage is preferred but has to be externally controlled. Or the opposite. Many/several encapsulated storages. In any case that is out of the scope of this design.

## **EmailRecord**

`EmailRecord` is the central, tagged type. Storage is a logical array
of `EmailRecord`s, plus a singly-linked free list threaded through the
same array for slot reuse — a specialized storage for `EmailRecord`s,
not a general one.

There is no table here, so no `ROWID` in the SQLite sense — but the
same distinction is worth naming, since it is exactly what trips
people up: every record carries **two** ids, with two different
lifetimes.

- `slot_id` — a plain array index, what CRUD keys every lookup on. Not
  a permanent identity: a deleted slot's index is reissued to the next
  `CreateEmail`, so the same numeric `slot_id` can belong to a
  different record over time.
- `unique_id` — assigned once on create, never reused, never looked up
  by. It exists purely so a record's identity stays legible even after
  its `slot_id` has been reissued. Its own type, `UniqueId`, is kept
  distinct from `slot_id`'s `EmailId` — the two ids answer different
  questions (which slot vs. which record), so sharing one type made
  that easy to miss.


**Synopsis**

```mermaid
classDiagram
    class EmailRecord {
        EmailId slot_id
        UniqueId unique_id
        text to
        text from
        text subject
        text body
    }
    note for EmailRecord "slot_id: on the storage implementation<br>unique_id: of the instance"
```


## EmailStorageResult

Standard return type is: `EmailStorageResult`. It is a tagged union, returning the `EmailRecord` or an error

**Synopsis**

```mermaid
classDiagram
    class EmailStorageResult {
        Tag tag
    }
    class ok {
        EmailRecord record
    }
    class err {
        text location
        text message
    }
    EmailStorageResult --> ok : tag == OK
    EmailStorageResult --> err : tag == ERR
    note for EmailStorageResult "tagged union <br> discriminated by Tag"
```

**Future improvements**. Notice we say "improvements" not "extensions".

- user configurable size of `location` and `message` char arrays, on the `err` struct.
  - that also allows for using char array parameters with size hint
- both `location` and `message` in a json format
  - Discuss: why not just one json formatted `payload`?

## EmailStorage

The core methods of the `EmailStorage` interface.

**Synopsis**

```mermaid
classDiagram
    class EmailStorage {
        EmailRecord[] records
        bool[] occupied
        size_t[] next_free_slot
        size_t free_list_head
        size_t high_water_mark
        size_t live_count
        CreateEmail(record) EmailStorageResult
        ReadEmail(slot_id) EmailStorageResult
        UpdateEmail(record) EmailStorageResult
        DeleteEmail(slot_id) EmailStorageResult
    }
    note for EmailStorage "records: the backing array<br>occupied[]: is this slot live right now<br>next_free_slot[]: free-list links, by index<br>free_list_head: index of most recently freed slot (LIFO list head)<br>high_water_mark: max slot index ever occupied<br>live_count: records currently in storage"
```

### Free Slots concept

Storage is a fixed-capacity array of `EmailRecord`s (see the `slot_id`
discussion under EmailRecord above for why a deleted slot's index is
reused rather than retired).

**Operation: Create**

```mermaid
%%{init: {'theme':'base', 'themeVariables': {'background':'#ffffff'}}}%%
sequenceDiagram
    participant Storage as EmailStorage
    participant Records_Array as records[]
    participant Bools_Array as occupied[]
    participant Free_Slots_Array as next_free_slot[]
    participant free_list_var as free_list_head
    participant high_wm_var as high_water_mark
    participant live_count_var as live_count

    Note over Storage: CreateEmail receives a newly<br>created email record as parameter

    alt IF a previously-deleted slot is waiting to be reused
        Storage->>free_list_var: take the slot at the front of the list --<br>use it as the slot for this new record
        Storage->>Free_Slots_Array: find out which slot, if any,<br>was queued up behind it
        Storage->>free_list_var: make that slot the new front of the list<br>(so it is offered next time)
    else IF no deleted slot is waiting -- storage still has<br>room it has never used before
        Storage->>high_wm_var: take the next never-used slot --<br>use it as the slot for this new record
        Storage->>high_wm_var: remember that this slot has now been used,<br>so it will not be handed out again this way
    end
    Storage->>Records_Array: write the new record into that slot<br>(its slot_id and unique_id are set here)
    Storage->>Bools_Array: mark that slot as occupied
    Storage->>live_count_var: add one to the count of live records
```

**Operation: Delete**

```mermaid
%%{init: {'theme':'base', 'themeVariables': {'background':'#ffffff'}}}%%
sequenceDiagram
    participant Storage as EmailStorage
    participant Records_Array as records[]
    participant Bools_Array as occupied[]
    participant Free_Slots_Array as next_free_slot[]
    participant free_list_var as free_list_head
    participant high_wm_var as high_water_mark
    participant live_count_var as live_count

    Note over Storage: DeleteEmail receives the slot_id<br>of the record to delete as parameter

    Storage->>Records_Array: fetch the record currently stored<br>at that slot_id, to hand back to the caller<br>as the deleted record on success
    Storage->>Bools_Array: mark that slot as no longer occupied
    Storage->>free_list_var: read whichever slot is currently<br>at the front of the free list
    Storage->>Free_Slots_Array: remember that slot behind the one<br>just freed, so it is not lost
    Storage->>free_list_var: put the just-freed slot at the front<br>of the list -- it will be offered first<br>to the next CreateEmail
    Storage->>live_count_var: subtract one from the count of live records
    Note over high_wm_var: untouched by delete --<br>only CreateEmail ever advances it
```


## Note on Multi Threading

Currently the design and code do not work in presence of multiple threads. That will be relatively straightforward to solve. We will use a single light mutex to be reachable from MailStorage public API and lock on entry unlock on leaving pattern. For that to be simple and resilient we will use the mandated compiler (GCC 15+) `defer` statement.
