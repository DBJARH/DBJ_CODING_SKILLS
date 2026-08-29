# gnu23, not c2x: toplevel/dbj_defer.h needs GNU nested functions, and
# this folder is built with the same flags as the rest of the repo.
# -Wswitch-enum, not just -Wswitch: -Wswitch goes silent the moment a
# `default` appears, which is exactly the mistake CLAUDE.md forbids.
CFLAGS    := -std=gnu23 -Wall -Wextra -Wswitch-enum -Werror -I ../toplevel -I .
# DBJ_BUILDS, if set in the environment, is where every folder's build
# output goes; falls back to ../builds (repo root) if unset.
BUILD_DIR := $(if $(DBJ_BUILDS),$(DBJ_BUILDS),../builds)
TARGET    := $(BUILD_DIR)/dbj_hashmap_smoketest

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

all: $(TARGET)

$(BUILD_DIR):
	$(MKDIR)

$(TARGET): dbj_hashmap_smoketest.c \
           dbj_hashmap.h \
           dbj_arena.h \
           dbj_hash_string.h \
           dbj_hashmap_element_result.h \
           ../toplevel/dbj_str.h \
           ../toplevel/dbj_result.h \
           ../toplevel/dbj_required_compile_time.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(TARGET) dbj_hashmap_smoketest.c

test: $(TARGET)
	$(TARGET)

clean:
	rm -f $(TARGET) $(TARGET).exe
