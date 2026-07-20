CFLAGS    := -std=c2x -Wall -Wswitch -Werror -I ../third_party/tau
# DBJ_BUILDS, if set in the environment, is where every folder's build
# output goes; falls back to ../builds (repo root) if unset.
BUILD_DIR := $(if $(DBJ_BUILDS),$(DBJ_BUILDS),../builds)
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

$(TARGET): dbj_str_test.c dbj_str.h | $(BUILD_DIR)
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
