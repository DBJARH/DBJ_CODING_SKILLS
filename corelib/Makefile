CFLAGS    := -std=c23 -Wall -Wextra -Wswitch -Werror -I.
# DBJ_BUILDS must be set in the environment and is where every folder's
# build output goes. Not optional and no fallback, as everywhere else in
# the repo. DBJ_CORELIB is not required here: this folder is corelib and
# compiles against itself with -I.
ifndef DBJ_BUILDS
$(error DBJ_BUILDS is not set -- point it at a build output folder)
endif
BUILD_DIR := $(subst \,/,$(DBJ_BUILDS))
TARGET    := $(BUILD_DIR)/corelib_smoke_test

# Windows/MinGW ships plain gcc.exe (gcc 15+); Linux distros still default
# `gcc` to an older version, so pin the versioned binary there.
# `mkdir -p` on both platforms, and deliberately outside the ifeq below.
# The cmd form `if not exist ... mkdir ...` is only correct when cmd runs
# the recipe, and make hands recipes to sh whenever one is on PATH -- which
# it is under Git Bash. sh cannot parse it and the build dies with
# "syntax error: unexpected end of file", but only on the run that has to
# create the directory, so an existing builds/ hides it completely.
MKDIR   := mkdir -p $(BUILD_DIR)

ifeq ($(OS),Windows_NT)
CC      := gcc
else
CC      := gcc-15
endif

.PHONY: all clean

all: $(TARGET)

$(BUILD_DIR):
	$(MKDIR)

$(TARGET): corelib_smoke_test.c dbj_defer.h dbj_result.h dbj_simple_log.h dbj_macros.h dbj_clintro.h dbj_required_compile_time.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(TARGET) corelib_smoke_test.c

clean:
	rm -f $(TARGET) $(TARGET).exe
