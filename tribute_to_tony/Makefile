CFLAGS    := -std=c2x -Wall -Wswitch -Werror -I ../third_party/tau
BUILD_DIR := ../builds
TARGET    := $(BUILD_DIR)/dbj_email_crud

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

$(TARGET): dbj_email_crud.c dbj_email_record.h dbj_email_storage_result.h dbj_email_storage.h ../toplevel/simple_log.h ../third_party/inifile/inifile.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(TARGET) dbj_email_crud.c

clean:
	rm -f $(TARGET) $(TARGET).exe
