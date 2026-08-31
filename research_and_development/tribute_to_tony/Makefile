# DBJ_CORELIB must be set in the environment and must point at the
# repo's corelib/ folder. It is not optional and there is no fallback:
# a wrong or missing corelib is a broken build, not a silent one.
ifndef DBJ_CORELIB
$(error DBJ_CORELIB is not set -- point it at <repo>/corelib)
endif

# Forward slashes throughout. make hands its recipes to sh where one is
# on PATH, and sh eats the backslashes out of a Windows path.
CORELIB   := $(subst \,/,$(DBJ_CORELIB))
# DBJ_BUILDS must be set in the environment and is where every folder's
# build output goes. Like DBJ_CORELIB it is not optional and has no
# fallback: the two say where the repo reads from and writes to, and a
# rule that holds for one holds for the other.
ifndef DBJ_BUILDS
$(error DBJ_BUILDS is not set -- point it at a build output folder)
endif
BUILD_DIR := $(subst \,/,$(DBJ_BUILDS))

CFLAGS    := -std=c2x -Wall -Wswitch -Werror -I ../../third_party/tau -I $(CORELIB)
TARGET    := $(BUILD_DIR)/dbj_email_crud

# Windows/MinGW ships plain gcc.exe (gcc 15+); Linux distros still default
# `gcc` to an older version, so pin the versioned binary there.
# `mkdir -p` on both platforms, and deliberately outside the ifeq. The cmd
# form `if not exist ... mkdir ...` is only correct when cmd runs the
# recipe, and make hands recipes to sh whenever one is on PATH -- which it
# is under Git Bash. sh cannot parse it and the build dies with "syntax
# error: unexpected end of file", but only on the run that has to create
# the directory, so an existing builds/ hides it completely.
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

$(TARGET): dbj_email_crud.c dbj_email_record.h dbj_email_storage_result.h dbj_email_storage.h $(CORELIB)/dbj_simple_log.h $(CORELIB)/dbj_clintro.h ../../third_party/inifile/inifile.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(TARGET) dbj_email_crud.c

clean:
	rm -f $(TARGET) $(TARGET).exe
