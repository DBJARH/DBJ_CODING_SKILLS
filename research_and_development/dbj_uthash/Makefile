# uthash.h is vendored in src/, so the benchmark builds with no clone
# present. Override to build against a different copy:
#
#     mingw32-make bench UTHASH_DIR=/some/where/uthash/src
#
UTHASH_DIR ?= src

# gnu23 to match the rest of the repo. uthash.h is third-party and does
# not build clean under this repo's warning set, so it is reached with
# -isystem rather than -I: warnings from a system header are suppressed,
# and our own code stays under -Werror.
# DBJ_CORELIB must be set in the environment and must point at the
# repo's corelib/ folder. It is not optional and there is no fallback:
# a wrong or missing corelib is a broken build, not a silent one.
ifndef DBJ_CORELIB
$(error DBJ_CORELIB is not set -- point it at <repo>/corelib)
endif

# Forward slashes throughout. make hands its recipes to sh where one is
# on PATH, and sh eats the backslashes out of a Windows path.
CORELIB   := $(subst \,/,$(DBJ_CORELIB))
# DBJ_BUILDS, if set in the environment, is where every folder's build
# output goes; falls back to ../../builds (repo root) if unset.
BUILD_DIR := $(subst \,/,$(if $(DBJ_BUILDS),$(DBJ_BUILDS),../../builds))

CFLAGS    := -std=gnu23 -Wall -Wextra -Wswitch-enum -Werror \
             -I $(CORELIB) -I ../../third_party/dbj_ubenchtest \
             -isystem $(UTHASH_DIR)
BENCH_TARGET := $(BUILD_DIR)/uthash_benchmark

# Windows/MinGW ships plain gcc.exe (gcc 15+); Linux distros still default
# `gcc` to an older version, so pin the versioned binary there.
ifeq ($(OS),Windows_NT)
CC      := gcc
MKDIR   := if not exist $(subst /,\,$(BUILD_DIR)) mkdir $(subst /,\,$(BUILD_DIR))
else
CC      := gcc-15
MKDIR   := mkdir -p $(BUILD_DIR)
endif

.PHONY: all clean bench

all: $(BENCH_TARGET)

$(BUILD_DIR):
	$(MKDIR)

# -O2: timing an unoptimised build measures something nobody ships.
$(BENCH_TARGET): uthash_benchmark.c \
           ../../third_party/dbj_ubenchtest/dbj_ubenchtest.h \
           $(CORELIB)/dbj_clintro.h \
           $(CORELIB)/dbj_required_compile_time.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -O2 -o $(BENCH_TARGET) uthash_benchmark.c

bench: $(BENCH_TARGET)
	$(BENCH_TARGET)

clean:
	rm -f $(BENCH_TARGET) $(BENCH_TARGET).exe
