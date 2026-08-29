CFLAGS    := -std=c23 -Wall -Wextra -Wswitch -Werror -I.
# DBJ_BUILDS, if set in the environment, is where every folder's build
# output goes; falls back to ../builds (repo root) if unset.
BUILD_DIR := $(if $(DBJ_BUILDS),$(DBJ_BUILDS),../builds)
TARGET    := $(BUILD_DIR)/toplevel_smoke_test

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

$(TARGET): toplevel_smoke_test.c dbj_defer.h dbj_result.h dbj_simple_log.h dbj_macros.h dbj_clintro.h dbj_required_compile_time.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(TARGET) toplevel_smoke_test.c

clean:
	rm -f $(TARGET) $(TARGET).exe
