/*
    2026JUL08       (c) dbj@dbj.org

    gcc -std=c2x -Wall -Wswitch -Werror -I ../third_party/tau -I ../toplevel -o dbj_str_test dbj_str_test.c

    Basic CRUD tests for the DEFINE_DBJSTR_TYPE macro in dbj_str.h,
    using tau (see third_party/tau) the same way dbj_email_crud.c does.
*/
#include <string.h>

#include <dbj_str.h>

DEFINE_DBJSTR_TYPE(str16, 16)

#include <tau/tau.h>
TAU_MAIN()

TEST(dbj_str, create_copies_source_bytes) {
    unsigned char src[16] = "hello";
    str16 s = str16_create(src);
    REQUIRE_TRUE(memcmp(s.data, src, 16) == 0, "create must copy all sz bytes");
}

TEST(dbj_str, read_roundtrips_data) {
    unsigned char src[16] = "roundtrip";
    str16 s = str16_create(src);

    unsigned char dest[16] = {0};
    str16_read(&s, dest);
    REQUIRE_TRUE(memcmp(dest, src, 16) == 0, "read must yield back what create stored");
}

TEST(dbj_str, update_replaces_data) {
    unsigned char first[16] = "first";
    unsigned char second[16] = "second";

    str16 s = str16_create(first);
    s = str16_update(s, second);

    unsigned char dest[16] = {0};
    str16_read(&s, dest);
    REQUIRE_TRUE(memcmp(dest, second, 16) == 0, "update must overwrite prior data");
    CHECK_TRUE(memcmp(dest, first, 16) != 0, "update must not leave the old data behind");
}

TEST(dbj_str, delete_zeroes_data) {
    unsigned char src[16] = "to be erased";
    str16 s = str16_create(src);
    s = str16_delete(s);

    unsigned char zero[16] = {0};
    unsigned char dest[16];
    str16_read(&s, dest);
    REQUIRE_TRUE(memcmp(dest, zero, 16) == 0, "delete must zero-fill the data");
}
