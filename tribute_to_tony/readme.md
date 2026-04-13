# Is AI coding feasible?

> The purpose of this repo is simple code and simple use-case that shows how LLM made bad and workable version. Very fast of course.
>
> Human has made much better version. Not that fast. 

## Introduction

* Simple to code but crucial to understand computer science concept, [resurfaced in 2026](https://youtu.be/wo84LFzx5nI)
* Compare vibed version one, with human improved version, started from the vibe coded version.
  * Human is me: dbj@dbj.org. I spent two maybe three hours over two days, also working on other tasks.

1. Vibe coded version: [text](ai_tribute_to_tony.c)
1. Human improved version: [text](tribute_to_tony.c)
1. AI made comparison, human assisted: [text](code_review.md)


## The Concept That Took 58 Years to Win

> [!NOTE]
> Actually Tony's "tagged union" was never lost. But it took wide audience 58 years to notice it. [Again](https://youtu.be/wo84LFzx5nI).

## The beginning: Hoare 1966

In 1966, Tony Hoare published "Record Handling" — a paper about how to organise data in programs. He was not thinking about objects, messages, or inheritance. He was thinking about something much simpler:

> A record has a **type tag**. Code **switches on that tag**.
> The compiler can check that every case is handled.

That is it. A struct with an enum field, and a switch that the compiler can audit. Explicit, inspectable, exhaustive.

Hoare called the construct `inspect`. The tag was visible data. The dispatch was visible code. Nothing was hidden.

---

## What happened next: Simula 1967 read Hoare and dropped the key part

Simula 67 — the first object-oriented language — was directly inspired by Hoare's paper. Its authors, Dahl and Nygaard, kept the idea of grouping data with behaviour, but they dropped `inspect`. They replaced exhaustive switching on a visible tag with **subclass polymorphism**: the receiver object decides what to do, invisibly, via a pointer lookup table (the vtable).

That single omission — losing the compiler-checked exhaustive switch — is what programmer and game developer Casey Muratori later called the
**35-year mistake**.

---

## Alan Kay, Smalltalk, and "message passing"

Alan Kay built Smalltalk in the 1970s and coined the term
**object-oriented programming**. His mental model was biological: cells that communicate by sending messages. The receiver owns the response. The caller is blind to the mechanism.

Kay's dispatch: `object.method(args)` — the vtable is hidden behind the dot.

This is elegant, but it has a cost: you cannot ask the compiler to
verify that every possible receiver type is handled. The switch is gone. The exhaustion guarantee is gone.



## C++, Java, and three decades of OOP hype

C++ inherited Simula's model. Java inherited C++. By the 1990s OOP was sold as the solution to every software problem. Textbooks, universities, and industry all converged on the same vocabulary: classes, inheritance,polymorphism, encapsulation.

The problems were real but quieter:

- Vtable dispatch is invisible — a bug in dispatch is nearly impossible
  to reason about statically
- Inheritance hierarchies grow brittle over time; changing a base class
  breaks subclasses in unpredictable ways
- The compiler cannot tell you that you forgot to handle a new subtype
- "Everything is an object" led to pathological designs (Java's
  `AbstractSingletonProxyFactoryBean`)

The industry spent roughly 35 years discovering these problems through painful experience.

---

## Discriminated unions return

The languages that came after began quietly restoring what Simula had dropped.

- **ML and Haskell** kept algebraic data types (tagged unions) and
  pattern matching with exhaustion checking throughout — but stayed largely academic
- **Rust** (2015) brought the same idea to systems programming with
  `enum` + `match`. The compiler refuses to compile a `match` that
  omits a variant. Hoare's `inspect` — under a different name — was back
- **Swift**, **Kotlin**, and **TypeScript** followed with sealed types
  and exhaustive `when`/`switch` expressions

Rust's `enum` + `match` is, structurally, exactly what Hoare described in 1966. The 58-year detour through OOP brought the field back to the starting point, with better syntax and a faster compiler.

## Hello World 

> [!NOTE] This is a very simple "tagged unions hello world" inline code example. 

Also used in [tribute_to_tony](tribute_to_tony.c) in this folder.

```C
typedef enum : uint8_t { OK, ERR } ResultTag;

// struct holding a tagged union
typedef struct {
    ResultTag tag;
    union {
        struct { EmailId id;    } ok;
        struct { char msg[128]; } err;
    };
} Result;
```

Make sure the GCC switch `-Wswitch` is used, and make sure `default` inside switch is not used. Read on.

**Why a tagged union beats the OOP equivalent**

The OOP version of `Result` looks like this: a `Result` base class, with `ResultOK` and `ResultERR` as subclasses. The caller gets back a `Result*` and must `dynamic_cast` or `instanceof`-check to know which one it actually is.

Three facts:

**1. The tag is hidden inside the C++ vtable.** In the C struct, `tag` is a plain field — visible, inspectable, printable. In the OOP version the type tag is the vtable pointer, buried inside the object. You cannot switch on it directly. You must call a virtual method or cast (and hope).

**2. The compiler can check for missed tag** — both C and C++ GCC/Clang support `-Wswitch -Werror` on enums. Forgetting a case is a build error. With a class hierarchy, adding a third subclass `ResultTimeout` silently compiles — every existing `instanceof` chain just falls through to the else. The switch is not C-exclusive, but the OOP alternative has no equivalent mechanism at all.

```c
typedef enum : uint8_t { OK, ERR, ERR_TIMEOUT } ResultTag;

typedef struct {
    ResultTag tag;
    union {
        struct { EmailId id;    } ok;
        struct { char msg[128]; } err;
    };
} Result;

// Caller code can not be wrong by accident of forgeting
// Add ERR_TIMEOUT to ResultTag and forget it in the switch — build fails:
// compilation error: enumeration value 'ERR_TIMEOUT' not handled in switch [-Werror=switch]
switch (r.tag) {
    case OK:  ...
    case ERR: ...
    // compile time error here
    // ERR_TIMEOUT missing → compiler err's and stops here
    // do not use default, ever
}
```
> [!IMPORTANT] switch default defeats the whole point. With default the compiler has nothing to warn about; ERR_TIMEOUT silently falls into default and you never know. **-Wswitch** only fires when there is no default. The rule is: no default, ever.

**3. The union — flat memory, no heap.** The C `union` inside `Result` guarantees `ok` and `err` occupy the **same memory**. The entire `Result` is one flat value — no heap, no pointer, known size at compile time. C++ has `union` too, but restricts it for non-trivial types and pushes you toward `std::variant`, which is a library workaround for something C expresses directly. The OOP alternative — a `Result` base class with `ResultOK` and `ResultERR` subclasses — allocates each on the heap separately; the caller holds a pointer and has no size guarantee at all.

The tagged union is not a pattern bolted on top of C. It *is* the C language's simple data model — a byte for the tag, bytes for the payload, nothing else. Quick example"

```c
Result find_email(EmailStorage *s, EmailId id) {
    {
        // signal email is found and which email is found
        return (Result){ .tag = OK, .ok = { .id = id } };
    }
    // signal the ERR, with the error message
    return (Result){ .tag = ERR, .err = { .msg = "not found" } };
}

// caller uses tag, available right inside the struct
Result r = find_email(&storage, 42);
switch (r.tag) {
    case OK:  printf("found: %d\n", r.ok.id);  break;
    case ERR: printf("error: %s\n", r.err.msg); break;
    // reminder: do not use 'default'
    // if ERR_TIMEOUT is added but not used, compilation will stop
    // because compiler switch `-Werror=switch` is used
}
```

## What proof of concept (POC) demonstrates

> [!NOTE] This folder contains is a bit more complicated [C23 example](tribute_to_tony.c). It is not "tagged unions hello world" kind of a code. 

`tribute_to_tony.c` is a POC of Hoare's idea in plain ISO C23:

```C
EmailCmd          — a record with a type tag (CmdTag)
email_dispatch    — a switch on that tag
impl_create/read/update/delete — plain functions, storage + params in, result out
```

No object. No `this`. No vtable. No inheritance.

The flag `-Wswitch -Werror` makes the compiler enforce exhaustion: add a
new `CmdTag` variant and forget to handle it in `email_dispatch`, and
the build fails. That is Hoare's `inspect` in 2024 C.

The `Result` type is a second discriminated union — `OK` or `ERR` —
with the `UNWRAP` macro guarding the happy path. Same principle: a tag,
a union, explicit handling.



## The irony

The OOP movement claimed to be about modelling the real world. The
alternative was "displaced" — Hoare's tagged record with an exhaustive switch — was never given a catchy name or a marketing campaign. It survived only in functional languages and in the habits of programmers who had understood the 1966 paper.

When Rust made `enum` + `match` central to the language, it was not inventing something new. It was recovering something proven from 1966.

Tony Hoare got there first.

---

*References*

- [C.A.R. Hoare, "Record Handling", 1966](https://dl.acm.org/doi/10.5555/1061032.1061041)
- [Ole-Johan Dahl & Kristen Nygaard, "SIMULA 67 Common Base Language", 1968](https://softwarepreservation.computerhistory.org/ALGOL/manual/Simula-CommonBaseLanguage.pdf)
- [Alan Kay, "The Early History of Smalltalk", 1993](https://dl.acm.org/doi/10.1145/155360.155364)
- [Casey Muratori, "The Big OOPs: Anatomy of a Thirty-Five Year Mistake", 2025](https://www.computerenhance.com/p/the-big-oops-anatomy-of-a-thirty)
- [The Rust Reference — `enum` and `match`](https://doc.rust-lang.org/reference/items/enumerations.html)
