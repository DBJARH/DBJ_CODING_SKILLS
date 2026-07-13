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

typedef struct {
    int  id;
    char name[64];
} Midget;

#define DBJ_MAKERESULT_IMPLEMENTATION
#include "dbj_result.h"

// -----------------------------------------------------------------------------
DBJ_MAKERESULT(Widget);
DBJ_MAKERESULT(Midget);

// -----------------------------------------------------------------------------
static WidgetResult make_widget(int id, const char name[static 1]) {
    if (id < 0)
        return Widget_make_err(__func__, "negative id");

    Widget w = {.id = id};
    snprintf(w.name, sizeof w.name, "%s", name);
    return Widget_make_ok(w);
}
// -----------------------------------------------------------------------------
static MidgetResult make_midget(int id, const char name[static 1]) {
    if (id < 0)
        return Midget_make_err(__func__, "negative id");

    Midget m = {.id = id};
    snprintf(m.name, sizeof m.name, "%s", name);
    return Midget_make_ok(m);
}

#define MAKE_RESULT(v_) _Generic((v_),  \
    Widget: Widget_make_ok,             \
    Midget: Midget_make_ok)(v_)
// -----------------------------------------------------------------------------
static void show_widget_result(WidgetResult r) {
    switch (r.tag) {
        case DBJ_RESULT_OK:
            SIMPLE_LOG("ok: id=%d name=%s", r.Widget_OK.my_value.id, r.Widget_OK.my_value.name);
            break;
        case DBJ_RESULT_ERR:
            SIMPLE_LOG("err: location=%s message=%s", r.Widget_ERR.location, r.Widget_ERR.message);
            break;
    }
}

static void show_midget_result(MidgetResult r) {
    switch (r.tag) {
        case DBJ_RESULT_OK:
            SIMPLE_LOG("ok: id=%d name=%s", r.Midget_OK.my_value.id, r.Midget_OK.my_value.name);
            break;
        case DBJ_RESULT_ERR:
            SIMPLE_LOG("err: location=%s message=%s", r.Midget_ERR.location, r.Midget_ERR.message);
            break;
    }
}

#define SHOW_RESULT(x_) _Generic((x_),        \
    WidgetResult: show_widget_result,         \
    MidgetResult: show_midget_result)(x_)
// -----------------------------------------------------------------------------
static int test_one(const char * argv[static 1]) {

    defer {
        SIMPLE_LOG("test_one( argv[0]: %s ): leaving scope", argv[0]);
    }

    SHOW_RESULT(make_widget(1, "gizmo"));
    SHOW_RESULT(make_midget(1, "gizmo-jr"));

    SHOW_RESULT(make_widget(-1, "bad"));
    SHOW_RESULT(make_midget(-1, "bad-jr"));

    SHOW_RESULT(MAKE_RESULT(((Widget){.id = 2, .name = "widget-two"})));
    SHOW_RESULT(MAKE_RESULT(((Midget){.id = 2, .name = "midget-two"})));

    SHOW_RESULT(Midget_make_err(__func__, "bad midget"));

    return 0;
}

int main (const int argc, const char * argv[static argc + 1])
{
    return test_one(argv);
}
