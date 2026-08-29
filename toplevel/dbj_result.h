#pragma once

typedef enum : unsigned short
{
    DBJ_RESULT_OK,
    DBJ_RESULT_ERR,
} DBJResultTag;

#define DBJ_RESULT_LOCATION_SIZE 512
#define DBJ_RESULT_MESSAGE_SIZE 512

/*

Macro to create generic result type holding the concrete type are macro (template) argument

usage synopsis:

struct MyType { bool active; char[128] name; }

DBJ_MAKERESULT( MyType ) ;

Above will create MyTypeResult as bellow

Resulting type synopsys:

typedef struct MyTypeResult MyTypeResult;

struct MyTypeResult {
    DBJResultTag tag;
    union {
        struct {
            MyType my_value;
        } MyType_OK;
        struct {
            char location[DBJ_RESULT_LOCATION_SIZE]; // * error origin, e.g. via __func__
            char message[DBJ_RESULT_MESSAGE_SIZE];   // json in the future, but not now
        } MyType_ERR;
    };
};

Factory functions generated alongside the type:

static inline MyTypeResult MyType_make_ok(MyType my_value_);   // in every TU
MyTypeResult MyType_make_err(const char *location_, const char *message);

NOTE: location_/message take plain `const char *`, not a `static N`
sized array -- a `static N` parameter asserts the caller passes at
least N bytes, which callers like `__func__` or short string literals
don't satisfy (confirmed by -Wstringop-overread when this was tried).
The fixed-size arrays are only for the *storage* fields inside the
struct, copied in via snprintf.

Declarations are always emitted, and so is the OK factory's body --
it is static inline. Only make_err's definition waits for the one
translation unit that defines DBJ_MAKERESULT_IMPLEMENTATION before the
DBJ_MAKERESULT(MyType) invocation -- same exactly-once-TU convention
as the rest of this repo's headers.

*/

#define DBJ_MAKERESULT_DECL(T_)                                                                \
    typedef struct T_##Result T_##Result;                                                      \
                                                                                               \
    struct T_##Result                                                                          \
    {                                                                                          \
        DBJResultTag tag;                                                                      \
        union                                                                                  \
        {                                                                                      \
            struct                                                                             \
            {                                                                                  \
                T_ my_value;                                                                   \
            } T_##_OK;                                                                         \
            struct                                                                             \
            {                                                                                  \
                char location[DBJ_RESULT_LOCATION_SIZE]; /* error origin, e.g. via __func__ */ \
                char message[DBJ_RESULT_MESSAGE_SIZE];   /* json in the future, but not now */ \
            } T_##_ERR;                                                                        \
        };                                                                                     \
    };                                                                                         \
                                                                                               \
    /* static inline, and emitted here rather than in the IMPL half:                            \
       the OK factory is two stores and needs neither snprintf nor                              \
       assert, so nothing forced it out of line. Out of line it cost a                           \
       real call returning the whole union by hidden pointer -- the                             \
       ERR arm's kilobyte included -- on a path that never touches the                          \
       ERR arm. Inline, the compiler sees the dead arm and drops it.                            \
       The err factory below stays out of line: it is the cold path,                            \
       and it is what the IMPLEMENTATION convention is for. */                                  \
    [[nodiscard]] static inline T_##Result T_##_make_ok(T_ my_value_)                           \
    {                                                                                          \
        return (T_##Result){                                                                   \
            .tag = DBJ_RESULT_OK,                                                              \
            .T_##_OK = {.my_value = my_value_}};                                               \
    }                                                                                          \
                                                                                               \
    T_##Result T_##_make_err(const char *location_, const char *message)

#define DBJ_MAKERESULT_IMPL(T_)                                                                      \
    /* (void) on each snprintf result, not merely the assert: with                                   \
       NDEBUG the assert vanishes and the variable is left unused,                                   \
       which -Werror=unused-variable rejects. The snprintf call                                      \
       itself is outside the assert, so it survives either way. */                                   \
    T_##Result T_##_make_err(const char *location_, const char *message)                             \
    {                                                                                                \
        T_##Result r_ = (T_##Result){.tag = DBJ_RESULT_ERR};                                         \
        int loc_rez_ = snprintf(r_.T_##_ERR.location, sizeof r_.T_##_ERR.location, "%s", location_); \
        assert(loc_rez_ >= 0 && (size_t)loc_rez_ < sizeof r_.T_##_ERR.location);                     \
        (void)loc_rez_;                                                                              \
        int msg_rez_ = snprintf(r_.T_##_ERR.message, sizeof r_.T_##_ERR.message, "%s", message);     \
        assert(msg_rez_ >= 0 && (size_t)msg_rez_ < sizeof r_.T_##_ERR.message);                      \
        (void)msg_rez_;                                                                              \
        return r_;                                                                                   \
    }

#ifdef DBJ_MAKERESULT_IMPLEMENTATION
#include <assert.h> /* assert, for T_##_make_err truncation check */
#include <stdio.h>  /* snprintf, for T_##_make_err */
#define DBJ_MAKERESULT(T_)   \
    DBJ_MAKERESULT_DECL(T_); \
    DBJ_MAKERESULT_IMPL(T_)
#else
#define DBJ_MAKERESULT(T_) \
    DBJ_MAKERESULT_DECL(T_)
#endif