#pragma once
/*
    2026APR04       (c) dbj@dbj.org

    Requires dbj_err.h (Result/ResultTag) and the EmailCmd type
    (EmailCrudCmdTag, CMD_CREATE/CMD_READ/CMD_UPDATE/CMD_DELETE)
    to be visible before use of DBG_LOG_RESULT / DBG_LOG_EMAIL_CMD.
*/
#include <stdio.h>

/* ── logging ───────────────────────────────────────────────────── */

#define my_LOG(fmt, ...) \
    fprintf(stdout, "[%6d] " fmt "\n", __LINE__ __VA_OPT__(, ) __VA_ARGS__)

#ifdef NDEBUG
#define my_DBG(fmt, ...) ((void)0)
#define DBG_LOG_RESULT(r_) ((void)0)
#define DBG_LOG_EMAIL_CMD(cmd_) ((void)0)
#else

#define my_DBG(fmt, ...)                              \
    fprintf(stdout, "[%6d] %s() " fmt "\n", __LINE__, \
            __func__ __VA_OPT__(, ) __VA_ARGS__)

#define DBG_LOG_RESULT(r_)                           \
    do {                                             \
        if ((r_).tag == OK)                          \
            my_DBG("result OK  id=%u", (r_).ok.id);  \
        else                                         \
            my_DBG("result ERR — %s", (r_).err.msg); \
    } while (0)

#define DBG_LOG_EMAIL_CMD(cmd_)                                              \
    do {                                                                     \
        switch ((cmd_)->tag) {                                               \
            case CMD_CREATE:                                                 \
                my_DBG("CMD CREATE to=%s from=%s subject=%s",                \
                       (cmd_)->params.create.to, (cmd_)->params.create.from, \
                       (cmd_)->params.create.subject);                       \
                break;                                                       \
            case CMD_READ:                                                   \
                my_DBG("CMD READ id=%u", (cmd_)->params.read.id);            \
                break;                                                       \
            case CMD_UPDATE:                                                 \
                my_DBG("CMD UPDATE id=%u subject=%s",                        \
                       (cmd_)->params.update.id,                             \
                       (cmd_)->params.update.subject);                       \
                break;                                                       \
            case CMD_DELETE:                                                 \
                my_DBG("CMD DELETE id=%u", (cmd_)->params.del.id);           \
                break;                                                       \
        }                                                                    \
    } while (0)
#endif
