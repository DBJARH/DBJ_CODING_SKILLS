> Original readme content

A simple .INI file parser written in C, released under the New BSD
license (see LICENSE). The original project home page is located at:

http://code.google.com/p/inih/

> New readme content

This code is copied from https://github.com/OSSystems/inih
The only substantial change we did is to make it into single header STB style

- `INIFILE_IMPLEMENTATION`
  - define this in exactly one translation unit (one `.c` file) before
    `#include "inifile.h"` to pull in the implementation; every other
    file just includes the header normally (declarations only)
- `INIFILE_ALLOW_MULTILINE`
  - nonzero (default) to allow multi-line value parsing, Python
    ConfigParser style: a subsequent indented line continues the
    previous `name`'s value
- `INIFILE_ALLOW_BOM`
  - nonzero (default) to allow a UTF-8 BOM (`0xEF 0xBB 0xBF`) at the
    start of the file
- `INIFILE_USE_STACK`
  - nonzero (default) to read each line into a stack buffer; zero to
    use the heap (`malloc`/`free`) instead
- `INIFILE_STOP_ON_FIRST_ERROR`
  - zero (default) to keep parsing after a line error; nonzero to stop
    at the first one
- `INIFILE_MAX_LINE`
  - maximum line length, in bytes, for any line in the `.ini` file
    (default `200`)


 
### FIXED (was: BAD DESIGN WE INHERITED)

`inifile.h` used to have:

```c
int ini_parse(const char* filename,
              int (*handler)(void* user, const char* section,
                             const char* name, const char* value),
              void* user);
```

The returned `int` was overloaded with three unrelated meanings -- 0
(success), a positive line number (where parsing first failed), or an
errno.h constant (a system-level failure) -- and nothing in the type
distinguished which case a caller got.

Fixed by splitting the two kinds of failure into separate fields,
in the spirit of this repo's Result pattern in `tribute_to_tony`:

```c
   typedef struct {
       errno_t      error_ ;
       int          optional_line_no_ ;
   } Inifile_result ;
```

`ini_parse` and `ini_parse_file` now both return `Inifile_result`.


---
2026-07-07  dbj@dbj.org

