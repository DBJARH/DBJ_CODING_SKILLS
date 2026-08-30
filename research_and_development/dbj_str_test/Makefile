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

CFLAGS    := -std=c2x -Wall -Wswitch -Werror -I ../../third_party/tau -I $(CORELIB)
TARGET    := $(BUILD_DIR)/dbj_str_test

# Windows/MinGW ships plain gcc.exe (gcc 15+); Linux distros still default
# `gcc` to an older version, so pin the versioned binary there.
ifeq ($(OS),Windows_NT)
CC      := gcc
MKDIR   := if not exist $(subst /,\,$(BUILD_DIR)) mkdir $(subst /,\,$(BUILD_DIR))
else
CC      := gcc-15
MKDIR   := mkdir -p $(BUILD_DIR)
endif

.PHONY: all run clean

all: $(TARGET)

$(BUILD_DIR):
	$(MKDIR)

$(TARGET): dbj_str_test.c $(CORELIB)/dbj_str.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(TARGET) dbj_str_test.c

run: $(TARGET)
	./$(TARGET)

ifeq ($(OS),Windows_NT)
clean:
	del /f /q $(subst /,\,$(TARGET)).exe 2>nul
else
clean:
	rm -f $(TARGET) $(TARGET).exe
endif
