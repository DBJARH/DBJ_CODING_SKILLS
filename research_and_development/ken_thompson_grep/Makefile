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

CFLAGS    := -std=c23 -Wall -Wextra -Wswitch -Werror -I $(CORELIB)
TARGET    := $(BUILD_DIR)/dbj_grep

# Windows/MinGW ships plain gcc.exe (gcc 15+); Linux distros still default
# `gcc` to an older version, so pin the versioned binary there.
ifeq ($(OS),Windows_NT)
CC      := gcc
MKDIR   := if not exist $(subst /,\,$(BUILD_DIR)) mkdir $(subst /,\,$(BUILD_DIR))
else
CC      := gcc-15
MKDIR   := mkdir -p $(BUILD_DIR)
endif

.PHONY: all clean

all: $(TARGET)

$(BUILD_DIR):
	$(MKDIR)

$(TARGET): dbj_grep_test.c dbj_grep.h $(CORELIB)/dbj_simple_log.h $(CORELIB)/dbj_clintro.h $(CORELIB)/dbj_required_compile_time.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(TARGET) dbj_grep_test.c

clean:
	rm -f $(TARGET) $(TARGET).exe
