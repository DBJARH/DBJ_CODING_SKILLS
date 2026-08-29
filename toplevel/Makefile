CFLAGS    := -std=c23 -Wall -Wextra -Wswitch -Werror -I.
# DBJ_BUILDS, if set in the environment, is where every folder's build
# output goes; falls back to ../builds (repo root) if unset.
BUILD_DIR := $(if $(DBJ_BUILDS),$(DBJ_BUILDS),../builds)
TARGET    := $(BUILD_DIR)/toplevel_smoke_test
STR_TARGET := $(BUILD_DIR)/dbj_str_smoketest

# Windows/MinGW ships plain gcc.exe (gcc 15+); Linux distros still default
# `gcc` to an older version, so pin the versioned binary there.
ifeq ($(OS),Windows_NT)
CC      := gcc
MKDIR   := if not exist $(subst /,\,$(BUILD_DIR)) mkdir $(subst /,\,$(BUILD_DIR))
else
CC      := gcc-15
MKDIR   := mkdir -p $(BUILD_DIR)
endif

.PHONY: all clean test

all: $(TARGET) $(STR_TARGET)

$(BUILD_DIR):
	$(MKDIR)

$(TARGET): toplevel_smoke_test.c dbj_defer.h dbj_result.h dbj_simple_log.h dbj_macros.h dbj_clintro.h dbj_required_compile_time.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(TARGET) toplevel_smoke_test.c

$(STR_TARGET): dbj_str_smoketest.c dbj_str.h dbj_required_compile_time.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(STR_TARGET) dbj_str_smoketest.c

test: $(STR_TARGET)
	$(STR_TARGET)

clean:
	rm -f $(TARGET) $(TARGET).exe $(STR_TARGET) $(STR_TARGET).exe
