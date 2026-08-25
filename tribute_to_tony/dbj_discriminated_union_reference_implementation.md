---
version: 0.1
---

# Discriminated Unions in C — Reference Implementation

A discriminated union is a `union` (the payload) plus a tag saying which
member is currently valid. This document builds one, and argues for the
one design decision that matters: **dispatch happens at compile time,
through `_Generic`, not at runtime through a function pointer.**

See [Vocabulary](#vocabulary) for terms.

## 1. The pattern

A `union` stores all members at the same address; only the last one
written may be read. The tag records which that was.

```c
typedef enum {
    TYPE_INT,
    TYPE_FLOAT
} DataType;

typedef struct {
    DataType type;
    union {
        int   int_val;
        float float_val;
    };
} Variant;
```

Reading is a `switch` on the tag:

```c
void variant_print(Variant const * var) {
    switch (var->type) {
        case TYPE_INT:   printf("%d\n", var->int_val);   break;
        case TYPE_FLOAT: printf("%f\n", var->float_val); break;
    }
}
```

That is the whole pattern. Everything below is about keeping it that
small.

## 2. Why not embed a function pointer

The tempting move is to carry the behaviour with the data — a `print`
member in the struct, assigned at construction, called as
`var->print(var)`. It looks like polymorphism and it costs more than it
gives.

**It cannot live in the union.** The pointer must be valid for every
tag, so it cannot share storage with the payload. It goes in the
surrounding struct, and every instance grows by a pointer — here, 8
bytes onto a struct whose payload is 4.

**It defeats the optimizer.** A `switch` over a dense enum compiles to a
jump table or, at these sizes, to a compare-and-branch the branch
predictor learns immediately. An indirect call through a struct member is
a load followed by a call to an address the compiler cannot see; it will
not inline the callee into it, and a mispredicted indirect branch stalls
the pipeline.

**It duplicates the tag.** The tag already says what the value is. A
function pointer says it a second time, and now two fields can disagree.
Nothing in the type system stops `{.type = TYPE_INT, .print =
print_float}`.

**It buys nothing back.** The usual argument for a vtable is open
extension — new variants added without touching existing code. A
discriminated union is *closed* by construction: the enum enumerates
every case. Paying vtable costs for extensibility the design explicitly
rejects is the worst of both.

So: no function pointer, in the union or outside it. Dispatch stays a
`switch`, and construction moves to compile time.

## 3. Construction, and the type problem

Construction is a factory returning by value — no `malloc`, no cleanup,
caller owns the storage:

```c
Variant variant_create(DataType type, double val);
```

This works only while every payload converts to one parameter type. Add
a `char const *` and it breaks: C has no overloading, so the honest
signature becomes

```c
Variant variant_create(DataType type, void const * val);   /* don't */
```

which is worse than it looks. The caller now passes a tag *and* a pointer
and nothing checks they agree — `variant_create(TYPE_INT, &some_float)`
compiles cleanly and reads a `float`'s bits as an `int`. The tag became a
promise the caller makes and the compiler never audits.

## 4. `_Generic` — the reference implementation

`_Generic` selects an expression by the **compile-time type** of its
controlling operand. That is exactly the information the `void const *`
signature threw away. Hand it the value and let it pick the constructor —
and with it, the tag:

```c
static Variant variant_create_int(int val) {
    return (Variant){ .type = TYPE_INT, .int_val = val };
}

static Variant variant_create_float(float val) {
    return (Variant){ .type = TYPE_FLOAT, .float_val = val };
}

#define variant_create(val) _Generic((val), \
    int:   variant_create_int,              \
    float: variant_create_float)(val)
```

Call sites carry no tag at all:

```c
Variant whole = variant_create(42);
Variant part  = variant_create(3.14f);
```

What this buys:

- **The tag is derived, never passed.** A mismatched tag is no longer a
  bug you can write. The class is gone, not diagnosed.
- **Nothing is stored to dispatch on.** `sizeof(Variant)` is tag plus
  payload; the selection left no runtime trace.
- **Unhandled types are compile errors.** `variant_create("text")` has no
  matching association and fails to compile.

The last point has a sharp edge worth knowing: association matching is
exact, after the usual argument conversions. `variant_create(3.14)` is a
`double`, matches neither branch, and is rejected. Add a `double:` branch
or write the `f`; either way the compiler tells you, which is the point.

**`_Generic` handles construction only.** Once the value is inside a
`Variant`, its compile-time type is gone — all that remains is
`var->type`, a runtime value. Reading is still the `switch` from §1, and
that is the correct division: compile time chooses the constructor,
runtime reads the tag.

## 5. Exhaustiveness

Write the `switch` with **no `default` case**:

```c
switch (var->type) {
    case TYPE_INT:   /* ... */ break;
    case TYPE_FLOAT: /* ... */ break;
}
```

Compile with:

```
gcc -std=c23 -Wall -Wextra -Werror
```

`-Wswitch` (in `-Wall`) warns when a `switch` over an enum omits a case
and has no `default`, and `-Werror` makes that fatal. Add a third tag and
every incomplete `switch` in the program fails the build, naming its
line.

A `default` case — `__builtin_unreachable()`, `assert(0)`, anything —
**silences that warning**. The `switch` becomes exhaustive as far as the
compiler is concerned, and the missing case survives to run time. The
empty space where `default` would go *is* the check. Leave it empty.

## 6. Complete reference implementation

```c
#include <stdio.h>

typedef enum {
    TYPE_INT,
    TYPE_FLOAT
} DataType;

typedef struct {
    DataType type;
    union {
        int   int_val;
        float float_val;
    };
} Variant;

static Variant variant_create_int(int val) {
    return (Variant){ .type = TYPE_INT, .int_val = val };
}

static Variant variant_create_float(float val) {
    return (Variant){ .type = TYPE_FLOAT, .float_val = val };
}

#define variant_create(val) _Generic((val), \
    int:   variant_create_int,              \
    float: variant_create_float)(val)

static void variant_print(Variant const * var) {
    switch (var->type) {
        case TYPE_INT:   printf("%d\n", var->int_val);   break;
        case TYPE_FLOAT: printf("%f\n", var->float_val); break;
    }
}

int main(int argc, char * argv[static argc + 1]) {
    (void)argc; (void)argv;

    Variant values[2] = {
        variant_create(42),
        variant_create(3.14f)
    };

    for (int i = 0; i < 2; ++i)
        variant_print(&values[i]);

    return 0;
}
```

No function pointers. No `void *`. No `default`. `sizeof(Variant)` is 8:
the tag and the payload, nothing else.

## Vocabulary

**Tag** — the enum member recording which union member is currently
valid. Also called the discriminant.

**Discriminated union** (tagged union, variant record) — a union paired
with its tag. Pascal called it a variant record; the C idiom is the
struct-wrapping-union above.

**`_Generic`** — C11 compile-time type selection. `_Generic(expr, T1: a,
T2: b)` evaluates to `a` or `b` according to the static type of `expr`,
which is never evaluated. Association matching is exact — no implicit
conversion from a `double` argument to a `float:` branch.

**Jump table** — a compiled `switch` over dense integer cases, indexing
an address table instead of testing cases in sequence.

**vtable** — the C++/OOP dispatch mechanism a per-instance function
pointer imitates: one indirection per call, paid for open extension. §2
is the argument that a closed union does not need it.

---

(c) 2026 by dbj@dbj.org | MIT license
