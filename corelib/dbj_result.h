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
                /* which kind of failure, in whatever enum the header                          \
                   that made this result type defines. 0 means the                             \
                   maker did not say. Only meaningful together with                            \
                   the result type -- code 1 of one type has nothing                           \
                   to do with code 1 of another. */                                            \
                unsigned short code;                                                           \
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
    T_##Result T_##_make_err(const char *location_, const char *message);                       \
                                                                                               \
    /* The same, plus which kind of failure it was. _make_err is this                           \
       with code 0 -- a caller that has nothing useful to put in the                            \
       code says nothing rather than inventing a value. */                                      \
    T_##Result T_##_make_err_coded(const char *location_, const char *message,                  \
                                   unsigned short code_)

#define DBJ_MAKERESULT_IMPL(T_)                                                                      \
    /* (void) on each snprintf result, not merely the assert: with                                   \
       NDEBUG the assert vanishes and the variable is left unused,                                   \
       which -Werror=unused-variable rejects. The snprintf call                                      \
       itself is outside the assert, so it survives either way. */                                   \
    T_##Result T_##_make_err_coded(const char *location_, const char *message,                       \
                                   unsigned short code_)                                             \
    {                                                                                                \
        T_##Result r_ = (T_##Result){.tag = DBJ_RESULT_ERR};                                         \
        r_.T_##_ERR.code = code_;                                                                    \
        int loc_rez_ = snprintf(r_.T_##_ERR.location, sizeof r_.T_##_ERR.location, "%s", location_); \
        assert(loc_rez_ >= 0 && (size_t)loc_rez_ < sizeof r_.T_##_ERR.location);                     \
        (void)loc_rez_;                                                                              \
        int msg_rez_ = snprintf(r_.T_##_ERR.message, sizeof r_.T_##_ERR.message, "%s", message);     \
        assert(msg_rez_ >= 0 && (size_t)msg_rez_ < sizeof r_.T_##_ERR.message);                      \
        (void)msg_rez_;                                                                              \
        return r_;                                                                                   \
    }                                                                                                \
                                                                                                     \
    T_##Result T_##_make_err(const char *location_, const char *message)                             \
    {                                                                                                \
        return T_##_make_err_coded(location_, message, 0);                                           \
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

/* ------------------------------------------------------------------
   Reading any result

   These four touch only `tag` and the ERR arm, both of which every
   generated result type has under the same names. So they are written
   once here rather than once per type -- a header that makes a result
   type only has to add the accessors for its own OK arm, whose member
   name it alone knows.
   ------------------------------------------------------------------ */

#define dbj_result_is_ok(result_) ((result_).tag == DBJ_RESULT_OK)

#define dbj_result_is_err(result_) ((result_).tag == DBJ_RESULT_ERR)

/* The ERR arm, valid only after dbj_result_is_err(). Both are
   fixed-size char arrays, so safe to print with %s. Named T_##_ERR in
   the struct, so these take the type as well -- there is no way to
   spell "whichever arm it is" in C. */
#define dbj_result_location(T_, result_) ((result_).T_##_ERR.location)

#define dbj_result_message(T_, result_) ((result_).T_##_ERR.message)

/* Which kind of failure, in the enum the header that made this result
   type defines. 0 when it did not say. This is the one a caller can
   switch on -- location and message are for a human. */
#define dbj_result_code(T_, result_) ((result_).T_##_ERR.code)