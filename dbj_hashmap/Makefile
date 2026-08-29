# gnu23, not c2x: toplevel/dbj_defer.h needs GNU nested functions, and
# this folder is built with the same flags as the rest of the repo.
# -Wswitch-enum, not just -Wswitch: -Wswitch goes silent the moment a
# `default` appears, which is exactly the mistake CLAUDE.md forbids.
CFLAGS    := -std=gnu23 -Wall -Wextra -Wswitch-enum -Werror -I ../toplevel -I .
# DBJ_BUILDS, if set in the environment, is where every folder's build
# output goes; falls back to ../builds (repo root) if unset.
BUILD_DIR := $(if $(DBJ_BUILDS),$(DBJ_BUILDS),../builds)
SMOKE_TARGET := $(BUILD_DIR)/dbj_hashmap_smoketest
BENCH_TARGET := $(BUILD_DIR)/dbj_hashmap_benchmarks

# Windows/MinGW ships plain gcc.exe (gcc 15+); Linux distros still default
# `gcc` to an older version, so pin the versioned binary there.
ifeq ($(OS),Windows_NT)
CC      := gcc
MKDIR   := if not exist $(subst /,\,$(BUILD_DIR)) mkdir $(subst /,\,$(BUILD_DIR))
else
CC      := gcc-15
MKDIR   := mkdir -p $(BUILD_DIR)
endif

.PHONY: all clean test bench

all: $(SMOKE_TARGET) $(BENCH_TARGET)

$(BUILD_DIR):
	$(MKDIR)

HEADERS := dbj_hashmap.h \
           dbj_hashmap_element_result.h \
           dbj_hashmap_element.h \
           dbj_hash_key.h \
           dbj_hash_string.h \
           dbj_arena.h \
           ../toplevel/dbj_str.h \
           ../toplevel/dbj_str_slice.h \
           ../toplevel/dbj_result.h \
           ../toplevel/dbj_required_compile_time.h

$(SMOKE_TARGET): dbj_hashmap_smoketest.c $(HEADERS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(SMOKE_TARGET) dbj_hashmap_smoketest.c

# -O2 for the benchmarks only: timing an unoptimised build measures
# something nobody ships.
$(BENCH_TARGET): dbj_hashmap_benchmarks.c $(HEADERS) \
                 ../toplevel/dbj_nanobench.h ../toplevel/dbj_clintro.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -O2 -o $(BENCH_TARGET) dbj_hashmap_benchmarks.c

test: $(SMOKE_TARGET)
	$(SMOKE_TARGET)

bench: $(BENCH_TARGET)
	$(BENCH_TARGET)

clean:
	rm -f $(SMOKE_TARGET) $(SMOKE_TARGET).exe $(BENCH_TARGET) $(BENCH_TARGET).exe
