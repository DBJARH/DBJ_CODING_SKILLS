/*
    2026JUL13       (c) dbj@dbj.org

    Smoke test for toplevel/defer.h, toplevel/dbj_result.h and
    toplevel/simple_log.h -- confirms the three compose: a factory
    method returning a DBJ_MAKERESULT-generated tagged union, logged
    with SIMPLE_LOG, with defer used for the obligatory cleanup-on-
    scope-exit demonstration.

    Build (GCC 15+ required, see required_compile_time.h):
        gcc -std=c23 -Wall -Wextra -Wswitch -Werror -o toplevel_smoke_test.exe toplevel_smoke_test.c
*/

#include "required_compile_time.h"

#include <stdio.h>
#include <string.h>

#include "defer.h"
#include "simple_log.h"

typedef struct {
    int  id;
    char name[64];
} Widget;

#define DBJ_MAKERESULT_IMPLEMENTATION
#include "dbj_result.h"

// -----------------------------------------------------------------------------
DBJ_MAKERESULT(Widget);

// -----------------------------------------------------------------------------
static WidgetResult make_widget(int id, const char name[static 1]) {
    if (id < 0)
        return Widget_make_err(__func__, "negative id");

    Widget w = {.id = id};
    snprintf(w.name, sizeof w.name, "%s", name);
    return Widget_make_ok(w);
}
// -----------------------------------------------------------------------------
static int test_one(const char * argv[static 1]) {

    defer {
        SIMPLE_LOG("test_one( argv[0]: %s ): leaving scope", argv[0]);
    }

    WidgetResult ok_result = make_widget(1, "gizmo");
    switch (ok_result.tag) {
        case DBJ_RESULT_OK:
            SIMPLE_LOG("ok: id=%d name=%s", ok_result.Widget_OK.my_value.id, ok_result.Widget_OK.my_value.name);
            break;
        case DBJ_RESULT_ERR:
            SIMPLE_LOG("unexpected err: %s", ok_result.Widget_ERR.message);
            break;
// -Wswitch makes GCC warn when a switch over an enum doesn't cover every enumerator, 
//  provided there's no default: case. 
//  Combined with -Werror, that warning becomes a hard compile error.            
    }

    WidgetResult err_result = make_widget(-1, "bad");
    switch (err_result.tag) {
        case DBJ_RESULT_OK:
            SIMPLE_LOG("unexpected ok: id=%d", err_result.Widget_OK.my_value.id);
            break;
        case DBJ_RESULT_ERR:
            SIMPLE_LOG("err: location=%s message=%s", err_result.Widget_ERR.location, err_result.Widget_ERR.message);
            break;
    }

    WidgetResult ok_again = Widget_make_ok((Widget){.id = 2, .name = "widget-two"});
    SIMPLE_LOG("ok_again: id=%d name=%s", ok_again.Widget_OK.my_value.id, ok_again.Widget_OK.my_value.name);

    return 0;
}

int main (const int argc, const char * argv[static argc + 1])
{
    return test_one(argv);
}
