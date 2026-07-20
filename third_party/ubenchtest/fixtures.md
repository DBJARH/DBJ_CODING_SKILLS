# Fixtures

> **DEPRECATED** -- see the note at the top of `ubenchtest.h`. Kept
> for reference only; `ubenchtest.h`/`.c` are not used by any active
> code in this repo.

Fixtures are used to define state that is initialized once and then
reused throughout a benchmark. This separates the cost of
initialization from the actual code you are measuring.

## How they work

Fixtures allow you to declare a struct that holds your testing state,
and use specific macros to set up (and optionally tear down) this
state before the benchmark runs.

The three core fixture macros used in the library are:

- `UBENCH_F(fixture_name, benchmark_name)` — defines the actual
  benchmark using the fixture.
- `UBENCH_F_SETUP(fixture_name)` — initializes the state for your
  fixture.
- `UBENCH_F_TEARDOWN(fixture_name)` — cleans up or frees any
  resources allocated during setup.

## Example implementation

Here is how you can set up and run a "fixtured" benchmark:

```c
#include "ubench.h"

// 1. Declare a struct that holds the state for your benchmark
struct my_fixture {
    int* data;
    size_t size;
};

// 2. Use UBENCH_F_SETUP to initialize your struct
UBENCH_F_SETUP(my_fixture) {
    // ubench_fixture is the automatically provided variable representing the struct instance
    ubench_fixture->size = 1000;
    ubench_fixture->data = malloc(sizeof(int) * ubench_fixture->size);
}

// 3. (Optional) Use UBENCH_F_TEARDOWN to clean up after the benchmark completes
UBENCH_F_TEARDOWN(my_fixture) {
    free(ubench_fixture->data);
}

// 4. Define the benchmark itself
UBENCH_F(my_fixture, process_data) {
    // Access your initialized fixture state via ubench_fixture
    for (size_t i = 0; i < ubench_fixture->size; i++) {
        ubench_fixture->data[i] = (int)i * 2;
    }
}

UBENCH_MAIN()
```
