#pragma once
/*
    2026APR04       (c) dbj@dbj.org

    Requires dbj_err.h (Result/ResultTag) and the EmailCmd type
    (EmailCrudCmdTag, CMD_CREATE/CMD_READ/CMD_UPDATE/CMD_DELETE)
    to be visible before use of DBG_LOG_RESULT / DBG_LOG_EMAIL_CMD.
*/
#include <stdio.h>

/* ── logging ───────────────────────────────────────────────────── */

#define SIMPLE_LOG(fmt, ...) \
    fprintf(stdout, "[%6d] " fmt "\n", __LINE__ __VA_OPT__(, ) __VA_ARGS__)

