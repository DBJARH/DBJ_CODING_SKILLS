# Deprecated corner: dbj_nanobench.h and its test, kept building in
# place. The header is local to this folder now; dbj_str.h and
# dbj_nth_prime.h still come from corelib.
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

CFLAGS    := -std=gnu23 -Wall -Wextra -Wswitch-enum -Werror -I . -I $(CORELIB)
TARGET    := $(BUILD_DIR)/dbj_nanobench_test

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

# -O2: timing an unoptimised build measures something nobody ships.
$(TARGET): dbj_nanobench_test.c dbj_nanobench.h \
           $(CORELIB)/dbj_str.h $(CORELIB)/dbj_nth_prime.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -O2 -o $(TARGET) dbj_nanobench_test.c

test: $(TARGET)
	$(TARGET)

clean:
	rm -f $(TARGET) $(TARGET).exe
