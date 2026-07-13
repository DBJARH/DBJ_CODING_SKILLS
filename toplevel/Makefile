CFLAGS    := -std=c23 -Wall -Wextra -Wswitch -Werror
BUILD_DIR := ../builds
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

$(TARGET): toplevel_smoke_test.c defer.h dbj_result.h simple_log.h required_compile_time.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(TARGET) toplevel_smoke_test.c

clean:
	rm -f $(TARGET) $(TARGET).exe
