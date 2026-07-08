/* inih -- simple .INI file parser

inih is released under the New BSD license (see LICENSE.txt). Go to the project
home page for more info:

http://code.google.com/p/inih/

*/

#ifndef __INI_H__
#define __INI_H__

/* Make this header file easier to include in C++ code */
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <errno.h>

#include "../../toplevel/defer.h"

/* Nonzero to allow multi-line value parsing, in the style of Python's
   ConfigParser. If allowed, ini_parse() will call the handler with the same
   name for each subsequent line parsed. */
#ifndef INIFILE_ALLOW_MULTILINE
#define INIFILE_ALLOW_MULTILINE 1
#endif

/* Nonzero to allow a UTF-8 BOM sequence (0xEF 0xBB 0xBF) at the start of
   the file. See http://code.google.com/p/inih/issues/detail?id=21 */
#ifndef INIFILE_ALLOW_BOM
#define INIFILE_ALLOW_BOM 1
#endif

/* Nonzero to use stack, zero to use heap (malloc/free). */
#ifndef INIFILE_USE_STACK
#define INIFILE_USE_STACK 1
#endif

/* Stop parsing on first error (default is to keep parsing). */
#ifndef INIFILE_STOP_ON_FIRST_ERROR
#define INIFILE_STOP_ON_FIRST_ERROR 0
#endif

/* Maximum line length for any line in INI file. */
#ifndef INIFILE_MAX_LINE
#define INIFILE_MAX_LINE 200
#endif

/* Storage size for the section/name buffers the handler callback is
   called with -- declared here (not just in the implementation block)
   so the [static N] contract on the handler's parameters below is
   visible to every includer, not only the translation unit that
   defines INIFILE_IMPLEMENTATION. */
#define MAX_SECTION 50
#define MAX_NAME 50

/* Parse given INI-style file. May have [section]s, name=value pairs
   (whitespace stripped), and comments starting with ';' (semicolon). Section
   is "" if name=value pair parsed before any section heading. name:value
   pairs are also supported as a concession to Python's ConfigParser.

   For each name=value pair parsed, call handler function with given user
   pointer as well as section, name, and value (data only valid for duration
   of handler call). Handler should return nonzero on success, zero on error.

   Returns an Inifile_result: error_ is 0 on success or an errno.h constant
   (ENOENT on file open error, ENOMEM on allocation error) on system failure;
   optional_line_no_ is the line number of the first parse error, or 0 if
   there was none. The two are reported separately on purpose -- see the
   FIXED note below.
*/
/* FIXED (was: BAD DESIGN WE INHERITED). ini_parse/ini_parse_file used to
   return a single int overloading three meanings -- success, a parse-error
   line number, and a system errno -- indistinguishable to the caller.
   Inifile_result now reports the system error and the parse-error line
   as separate fields. */

   typedef struct {
       errno_t      error_ ;
       int          optional_line_no_ ;
   } Inifile_result ;


Inifile_result ini_parse(const char* filename,
              int (*handler)(void* user, const char section[static MAX_SECTION],
                             const char name[static MAX_NAME],
                             const char value[static INIFILE_MAX_LINE]),
              void* user);

/* Same as ini_parse(), but takes an already-open FILE* instead of a
   filename, and -- unlike ini_parse(), which opens and closes the file
   itself -- never closes it; the caller opened it, the caller closes it. */
Inifile_result ini_parse_file(FILE* file,
                   int (*handler)(void* user, const char section[static MAX_SECTION],
                                  const char name[static MAX_NAME],
                                  const char value[static INIFILE_MAX_LINE]),
                   void* user);

#ifdef __cplusplus
}
#endif

/* define INIFILE_IMPLEMENTATION in exactly one translation unit (aka c file)
   before including this header, STB style, to get the implementation */
#ifdef INIFILE_IMPLEMENTATION

#include <ctype.h>
#include <string.h>

#if !INIFILE_USE_STACK
#include <stdlib.h>
#endif

/* Strip whitespace chars off end of given string, in place. Return s. */
static char* rstrip(char* s)
{
    char* p = s + strlen(s);
    while (p > s && isspace((unsigned char)(*--p)))
        *p = '\0';
    return s;
}

/* Return pointer to first non-whitespace char in given string. */
static char* lskip(const char* s)
{
    while (*s && isspace((unsigned char)(*s)))
        s++;
    return (char*)s;
}

/* Return pointer to first char c or ';' comment in given string, or pointer to
   null at end of string if neither found. ';' must be prefixed by a whitespace
   character to register as a comment. */
static char* find_char_or_comment(const char* s, char c)
{
    int was_whitespace = 0;
    while (*s && *s != c && !(was_whitespace && *s == ';')) {
        was_whitespace = isspace((unsigned char)(*s));
        s++;
    }
    return (char*)s;
}

/* Version of strncpy that ensures dest (size bytes) is null-terminated. */
static char* strncpy0(char* dest, const char* src, size_t size)
{
    strncpy(dest, src, size);
    dest[size - 1] = '\0';
    return dest;
}

/* See documentation in header file. */
Inifile_result ini_parse_file(FILE* file,
                   int (*handler)(void*, const char[static MAX_SECTION],
                                  const char[static MAX_NAME],
                                  const char[static INIFILE_MAX_LINE]),
                   void* user)
{
    /* Uses a fair bit of stack (use heap instead if you need to) */
#if INIFILE_USE_STACK
    char line[INIFILE_MAX_LINE] = {};
#else
    char* line = {};
#endif
    char section[MAX_SECTION] = "";
    char prev_name[MAX_NAME] = "";

    char* start = {};
    char* end = {};
    char* name = {};
    char* value = {};
    int lineno = {};
    int error = {};

#if !INIFILE_USE_STACK
    line = (char*)malloc(INIFILE_MAX_LINE);
    if (!line) {
        return (Inifile_result){.error_ = ENOMEM, .optional_line_no_ = 0};
    }
#endif

    /* Scan through file line by line */
    while (fgets(line, INIFILE_MAX_LINE, file) != NULL) {
        lineno++;

        start = line;
#if INIFILE_ALLOW_BOM
        if (lineno == 1 && (unsigned char)start[0] == 0xEF &&
                           (unsigned char)start[1] == 0xBB &&
                           (unsigned char)start[2] == 0xBF) {
            start += 3;
        }
#endif
        start = lskip(rstrip(start));

        if (*start == ';' || *start == '#') {
            /* Per Python ConfigParser, allow '#' comments at start of line */
        }
#if INIFILE_ALLOW_MULTILINE
        else if (*prev_name && *start && start > line) {
            /* Non-black line with leading whitespace, treat as continuation
               of previous name's value (as per Python ConfigParser). */
            if (!handler(user, section, prev_name, start) && !error)
                error = lineno;
        }
#endif
        else if (*start == '[') {
            /* A "[section]" line */
            end = find_char_or_comment(start + 1, ']');
            if (*end == ']') {
                *end = '\0';
                strncpy0(section, start + 1, sizeof(section));
                *prev_name = '\0';
            }
            else if (!error) {
                /* No ']' found on section line */
                error = lineno;
            }
        }
        else if (*start && *start != ';') {
            /* Not a comment, must be a name[=:]value pair */
            end = find_char_or_comment(start, '=');
            if (*end != '=') {
                end = find_char_or_comment(start, ':');
            }
            if (*end == '=' || *end == ':') {
                *end = '\0';
                name = rstrip(start);
                value = lskip(end + 1);
                end = find_char_or_comment(value, '\0');
                if (*end == ';')
                    *end = '\0';
                rstrip(value);

                /* Valid name[=:]value pair found, call handler */
                strncpy0(prev_name, name, sizeof(prev_name));
                if (!handler(user, section, name, value) && !error)
                    error = lineno;
            }
            else if (!error) {
                /* No '=' or ':' found on name[=:]value line */
                error = lineno;
            }
        }

#if INIFILE_STOP_ON_FIRST_ERROR
        if (error)
            break;
#endif
    }

#if !INIFILE_USE_STACK
    free(line);
#endif

    return (Inifile_result){.error_ = 0, .optional_line_no_ = error};
}

/* See documentation above. */
Inifile_result ini_parse(const char* filename,
              int (*handler)(void*, const char[static MAX_SECTION],
                             const char[static MAX_NAME],
                             const char[static INIFILE_MAX_LINE]),
              void* user)
{
    FILE* file = {};

    file = fopen(filename, "r");
    if (!file)
        return (Inifile_result){.error_ = ENOENT, .optional_line_no_ = 0};
    defer { fclose(file); }
    return ini_parse_file(file, handler, user);
}

#endif /* INIFILE_IMPLEMENTATION */

#endif /* __INI_H__ */
