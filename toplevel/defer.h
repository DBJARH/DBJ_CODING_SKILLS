
#ifndef GUSTEDT_DEFER
#define GUSTEDT_DEFER
/**
 * https://gustedt.wordpress.com/2025/01/06/simple-defer-ready-to-use/


 Implementing defer with gcc
With a minimal macro wrapper this feature works out of the box in gcc since at least two decades. Written with C23’s attribute feature the inner macro looks as simple as the following:

```c
#define __DEFER__(F, V)      \
  auto void F(int*);         \
  [[gnu::cleanup(F)]] int V; \
  auto void F(int*)
````
  Here the three lines of the macro are as follows:

 - auto void F(int*); forward-declares a nested (local) function F.
 - [[gnu::cleanup(F)]] int V; establishes this function as a cleanup handler of an auxiliary variable V
 - The second auto void F(int*) then starts the definition of the local function which is completed with the 
user’s compound statement as the function body.

For this to work we have to provide unique names F and V such that several defer blocks
 may appear within the same anchor block. This is ensured by another very common extension __COUNTER__

```c
#define defer __DEFER(__COUNTER__)
#define __DEFER(N) __DEFER_(N)
#define __DEFER_(N) __DEFER__(__DEFER_FUNCTION_ ## N, __DEFER_VARIABLE_ ## N)
```

That is basically it, a straight application of the [[gnu::cleanup]] feature that avoids the need for inventing safe identifiers,
avoids the definition of an one-shot function with internal or external linkage far away from its use,
integrates well and quite efficiently with the existing compiler infrastructure.
Indeed, when adding a bit more magic (such as [[gnu::always_inline]]) the assembly that is 
produced is very efficient and avoids function calls, trampolines and indirections. (See also Omar Anson’s 
    blog entry on how efficient the cleanup attribute seems to be implemented in gcc.)

If you don’t like or have the C23 attribute syntax yet, you should easily be able to use 
gcc’s legacy __attribute__((...)) syntax without problems.

*/

#include "required_compile_time.h"

#define __DEFER__(F, V)      \
  auto void F(int*);         \
  [[gnu::cleanup(F)]] int V; \
  auto void F(int*)

#define defer __DEFER(__COUNTER__)
#define __DEFER(N) __DEFER_(N)
#define __DEFER_(N) __DEFER__(__DEFER_FUNCTION_ ## N, __DEFER_VARIABLE_ ## N)

#endif // GUSTEDT_DEFER