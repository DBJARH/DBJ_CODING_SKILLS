# DBJ_CORELIB must be set in the environment and must point at the
# repo's corelib/ folder. It is not optional and there is no fallback:
# a wrong or missing corelib is a broken build, not a silent one.
ifndef DBJ_CORELIB
$(error DBJ_CORELIB is not set -- point it at <repo>/corelib)
endif

# gnu23, not c2x: corelib/dbj_defer.h needs GNU nested functions, and
# this folder is built with the same flags as the rest of the repo.
# -Wswitch-enum, not just -Wswitch: -Wswitch goes silent the moment a
# `default` appears, which is exactly the mistake CLAUDE.md forbids.
# Forward slashes throughout. make hands its recipes to sh where one is
# on PATH, and sh eats the backslashes out of a Windows path.
CORELIB   := $(subst \,/,$(DBJ_CORELIB))
# DBJ_BUILDS must be set in the environment and is where every folder's
# build output goes. Like DBJ_CORELIB it is not optional and has no
# fallback: the two say where the repo reads from and writes to, and a
# rule that holds for one holds for the other.
ifndef DBJ_BUILDS
$(error DBJ_BUILDS is not set -- point it at a build output folder)
endif
BUILD_DIR := $(subst \,/,$(DBJ_BUILDS))

CFLAGS    := -std=gnu23 -Wall -Wextra -Wswitch-enum -Werror -I $(CORELIB) -I . \
             -I ../../third_party/dbj_ubenchtest
SMOKE_TARGET := $(BUILD_DIR)/dbj_hashmap_smoketest
MAKE_TARGET  := $(BUILD_DIR)/dbj_make_hashmap_smoketest
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

all: $(SMOKE_TARGET) $(MAKE_TARGET) $(BENCH_TARGET)

$(BUILD_DIR):
	$(MKDIR)

HEADERS := dbj_hashmap.h \
           dbj_make_hashmap.h \
           dbj_hash_key.h \
           dbj_hash_string.h \
           dbj_arena.h \
           $(CORELIB)/dbj_str.h \
           $(CORELIB)/dbj_str_slice.h \
           $(CORELIB)/dbj_result.h \
           $(CORELIB)/dbj_required_compile_time.h

$(SMOKE_TARGET): dbj_hashmap_smoketest.c $(HEADERS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(SMOKE_TARGET) dbj_hashmap_smoketest.c

# dbj_make_hashmap.h depends on nothing in this folder but dbj_result.h --
# that is the point of it.
$(MAKE_TARGET): dbj_make_hashmap_smoketest.c dbj_make_hashmap.h \
                $(CORELIB)/dbj_result.h $(CORELIB)/dbj_required_compile_time.h \
                | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(MAKE_TARGET) dbj_make_hashmap_smoketest.c

# -O2 for the benchmarks only: timing an unoptimised build measures
# something nobody ships.
$(BENCH_TARGET): dbj_hashmap_benchmarks.c $(HEADERS) \
                 ../../third_party/dbj_ubenchtest/dbj_ubenchtest.h \
                 $(CORELIB)/dbj_clintro.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -O2 -o $(BENCH_TARGET) dbj_hashmap_benchmarks.c

test: $(SMOKE_TARGET) $(MAKE_TARGET)
	$(SMOKE_TARGET)
	$(MAKE_TARGET)

bench: $(BENCH_TARGET)
	$(BENCH_TARGET)

clean:
	rm -f $(SMOKE_TARGET) $(SMOKE_TARGET).exe $(BENCH_TARGET) $(BENCH_TARGET).exe
