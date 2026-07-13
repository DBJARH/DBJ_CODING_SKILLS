#pragma once

typedef enum : unsigned short {
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
            MyTypeResult (*make)(MyType my_value_);
            MyType my_value;
        } MyType_OK;
        struct {
            MyTypeResult (*make)(const char *location_, const char *message);
            char location[DBJ_RESULT_LOCATION_SIZE]; // * error origin, e.g. via __func__
            char message[DBJ_RESULT_MESSAGE_SIZE];   // json in the future, but not now
        } MyType_ERR;
    };
};

Factory functions generated alongside the type (mirrors the hand
written pattern in tribute_to_tony/dbj_email_storage_result.h):

MyTypeResult MyType_result_ok(MyType my_value_);
MyTypeResult MyType_result_err(const char *location_, const char *message);

NOTE: location_/message take plain `const char *`, not a `static N`
sized array -- a `static N` parameter asserts the caller passes at
least N bytes, which callers like `__func__` or short string literals
don't satisfy (confirmed by -Wstringop-overread when this was tried).
The fixed-size arrays are only for the *storage* fields inside the
struct, copied in via snprintf.

Declarations are always emitted. Definitions are only emitted in the
one translation unit that defines DBJ_MAKERESULT_IMPLEMENTATION before
the DBJ_MAKERESULT(MyType) invocation -- same exactly-once-TU
convention as the rest of this repo's headers.

*/

#define DBJ_MAKERESULT_DECL(T_)                                                                                        \
    typedef struct T_##Result T_##Result;                                                                              \
                                                                                                                        \
    struct T_##Result {                                                                                                \
        DBJResultTag tag;                                                                                              \
        union {                                                                                                        \
            struct {                                                                                                   \
                T_##Result (*make)(T_ my_value_);                                                                      \
                T_ my_value;                                                                                           \
            } T_##_OK;                                                                                                 \
            struct {                                                                                                   \
                T_##Result (*make)(const char *location_, const char *message);                                       \
                char location[DBJ_RESULT_LOCATION_SIZE]; /* error origin, e.g. via __func__ */                         \
                char message[DBJ_RESULT_MESSAGE_SIZE];   /* json in the future, but not now */                        \
            } T_##_ERR;                                                                                                \
        };                                                                                                             \
    };                                                                                                                 \
                                                                                                                        \
    T_##Result T_##_result_ok(T_ my_value_);                                                                           \
    T_##Result T_##_result_err(const char *location_, const char *message)

#define DBJ_MAKERESULT_IMPL(T_)                                                                        \
    T_##Result T_##_result_ok(T_ my_value_) {                                                          \
        return (T_##Result){                                                                           \
            .tag = DBJ_RESULT_OK,                                                                      \
            .T_##_OK = {.make = T_##_result_ok, .my_value = my_value_}};                                \
    }                                                                                                   \
                                                                                                         \
    T_##Result T_##_result_err(const char *location_, const char *message) { \
        T_##Result r_ = (T_##Result){                                                                   \
            .tag = DBJ_RESULT_ERR,                                                                      \
            .T_##_ERR = {.make = T_##_result_err}};                                                      \
        snprintf(r_.T_##_ERR.location, sizeof r_.T_##_ERR.location, "%s", location_);                   \
        snprintf(r_.T_##_ERR.message, sizeof r_.T_##_ERR.message, "%s", message);                        \
        return r_;                                                                                      \
    }

#ifdef DBJ_MAKERESULT_IMPLEMENTATION
#include <stdio.h> /* snprintf, for T_##_result_err */
#define DBJ_MAKERESULT(T_) \
    DBJ_MAKERESULT_DECL(T_); \
    DBJ_MAKERESULT_IMPL(T_)
#else
#define DBJ_MAKERESULT(T_) \
    DBJ_MAKERESULT_DECL(T_)
#endif