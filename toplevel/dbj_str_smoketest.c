/*
    2026AUG29       (c) dbj@dbj.org

    Smoke test for dbj_str.h -- the sized string types.

    The claim under test, in one line: no union is needed if one size
    suffices, and a second size means a second named type.

    That is not a design preference, it is what C permits. A struct's
    array bound is part of its type, so two sizes are two types --
    there is no `auto`, `_Generic` or macro that makes one type have
    two sizes. This file tries each of those and shows where it stops.

    Exits non-zero on failure, so `make test` is a usable check.
*/
#include <dbj_required_compile_time.h>

// #include <dbj_str.h>

#include <stdio.h>
#include <string.h>

#define CHECK(cond_, what_)                  \
    do                                       \
    {                                        \
        if (cond_)                           \
        {                                    \
            printf("  ok    %s\n", (what_)); \
        }                                    \
        else                                 \
        {                                    \
            printf("  FAIL  %s\n", (what_)); \
        }                                    \
    } while (0)

#pragma region dbj_simple_str

#define DEFINE_DBJSTR_SIMPLE(name, sz)                                             \
    typedef struct                                                               \
    {                                                                            \
        unsigned char data[sz];                                                  \
    } name;                                                                      \
                                                                                 \
    /* Create: Factory method with static constraint */                          \
    static inline name name##_create(const unsigned char src[static sz])         \
    {                                                                            \
        name s;                                                                  \
        memcpy(s.data, src, sz);                                                 \
        return s;                                                                \
    }                                                                            \
                                                                                 \
    /* Read: Direct access, data returned by value */                            \
    static inline void name##_read(const name *s, unsigned char dest[static sz]) \
    {                                                                            \
        memcpy(dest, s->data, sz);                                               \
    }                                                                            \
                                                                                 \
    /* Update: Functional modification, returning new data */                    \
    static inline name name##_update(name s, const unsigned char src[static sz]) \
    {                                                                            \
        memcpy(s.data, src, sz);                                                 \
        return s;                                                                \
    }                                                                            \
                                                                                 \
    /* Delete: Clear the data (Reset to default) */                              \
    static inline name name##_delete(name s)                                     \
    {                                                                            \
        memset(s.data, 0, sz);                                                   \
        return s;                                                                \
    }

#pragma endregion // dbj_simple_str

/* Two sizes, so two types. This is the whole mechanism -- there is no
   third thing to reach for. */
DEFINE_DBJSTR_SIMPLE(small, 8)
DEFINE_DBJSTR_SIMPLE(large, 32)

/* A named type is what lets a size cross a function boundary. An
   anonymous struct from a macro cannot: nothing could write this
   parameter's type. */
[[nodiscard]] static size_t capacity_of_small(small text)
{
    return sizeof(text.data);
}

int main(void)
{
    printf("dbj_str smoke test\n\n");

    /* ---- one size suffices: no union, no tag ---- */

    /* The type carries its own capacity. Nothing stores a length
       beside it, and nothing tags which size it is -- there is only
       the one.

       [[gnu::nonstring]]: these buffers hold bytes, not C strings --
       exactly 8, with no room for a NUL. GCC 15 warns without it
       (-Wunterminated-string-initialization), and the warning is
       right: nothing here may be passed to strlen or printf %s. */
    [[gnu::nonstring]] unsigned char source[8] = "abcdefgh";
    small one = small_create(source);

    CHECK(sizeof(one.data) == 8, "capacity comes from the type, not from a stored field");
    CHECK(memcmp(one.data, "abcdefgh", 8) == 0, "create copies the bytes");
    CHECK(sizeof(small) == 8, "the struct is exactly its array -- no tag, no padding");

    /* A zeroed value is the empty string, which is why no constructor
       is needed for that case. */
    small zeroed = {};
    CHECK(zeroed.data[0] == 0, "a zeroed value needs no constructor");

    /* CRUD round trip, by value throughout -- nothing points anywhere */
    [[gnu::nonstring]] unsigned char replacement[8] = "12345678";
    small updated = small_update(one, replacement);
    CHECK(memcmp(updated.data, "12345678", 8) == 0, "update returns a new value");
    CHECK(memcmp(one.data, "abcdefgh", 8) == 0, "update leaves the original alone");

    unsigned char readback[8] = {};
    small_read(&updated, readback);
    CHECK(memcmp(readback, "12345678", 8) == 0, "read copies out");

    small cleared = small_delete(updated);
    CHECK(cleared.data[0] == 0, "delete zeroes the value");

    /* ---- a second size means a second named type ---- */

    unsigned char wide[32] = "a thirty-two byte buffer, this.";
    large two = large_create(wide);

    CHECK(sizeof(two.data) == 32, "the second type has its own capacity");
    CHECK(sizeof(small) != sizeof(large), "two sizes are two distinct types");

    /* The types do not convert. Uncommenting either line below is a
       compile error, which is the point:

           one = two;                    incompatible types
           capacity_of_small(two);       incompatible parameter

       C offers no way around it. A struct's array bound is part of its
       type, so a function taking `small` cannot also take `large`. */
    CHECK(capacity_of_small(one) == 8, "a named type crosses a function boundary");

    /* ---- what a union would have bought, and what it costs ---- */

    /* Storing either size in one slot is the only thing a union adds.
       It is not free: the union is as large as its largest member, so
       a small value in there occupies the large one's space. */
    typedef struct
    {
        enum
        {
            IS_SMALL,
            IS_LARGE
        } tag;
        union
        {
            small as_small;
            large as_large;
        };
    } either;

    CHECK(sizeof(either) >= sizeof(large) + sizeof(int),
          "a union costs its largest member plus a tag, whatever it holds");

    either held = {.tag = IS_SMALL, .as_small = one};
    CHECK(sizeof(held.as_small.data) == 8 && sizeof(either) > 8,
          "a small value in a union still occupies the large one's space");

    /* ---- auto: infers a named type, will not invent one ---- */

    /* C23 auto infers the type of an object. With a named type it
       works and saves writing the name. */
    auto inferred = small_create(source);
    CHECK(sizeof(inferred.data) == 8, "auto infers a named sized type");

    /* What auto will NOT do is accept a type defined in its own
       initialiser. This does not compile:

           #define HS(lit_) (struct { char data[sizeof(lit_)]; }){ lit_ }
           auto s = HS("A");

       GCC 15: "'struct <anonymous>' defined in underspecified object
       initializer". C23 forbids it deliberately -- the type would
       exist nowhere else in the program, so nothing could name it,
       and no function could take or return it.

       That is why DEFINE_DBJSTR_SIMPLE names its types. A size that
       follows its initialiser is possible only for a value that never
       leaves the scope it was made in, which is no use to a map. */

    /* ---- _Generic dispatches on type, so it separates the two ---- */

/* This works because small and large ARE different types. It would
   have nothing to dispatch on if one type served both sizes -- which
   is the same fact from the other side. */
#define capacity_of(x_) _Generic((x_), small: 8, large: 32)

    CHECK(capacity_of(one) == 8 && capacity_of(two) == 32,
          "_Generic tells the two types apart");

    /* What _Generic cannot do is pick a size for a factory: every
       factory takes the same argument type, so there is nothing in
       the call to dispatch on. The size is not an argument, it is the
       answer -- and C selects a return type by function name, not by
       inference. */

    printf("\n%s: %d failure(s)\n", failures ? "FAILED" : "PASSED", failures);
    return failures ? 1 : 0;
}
