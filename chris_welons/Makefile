# gnu23, not c2x: toplevel/dbj_defer.h needs GNU nested functions.
CFLAGS    := -std=gnu23 -Wall -Wextra -Wswitch -Werror -I ../toplevel
# DBJ_BUILDS, if set in the environment, is where every folder's build
# output goes; falls back to ../builds (repo root) if unset.
BUILD_DIR := $(if $(DBJ_BUILDS),$(DBJ_BUILDS),../builds)
TARGET    := $(BUILD_DIR)/dbj_arena_hashmap_hashtrie

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

$(TARGET): dbj-arena-hashmap-hashtrie.c \
           ../toplevel/dbj_str_slice.h \
           ../toplevel/dbj_clintro.h \
           ../toplevel/dbj_defer.h \
           ../toplevel/dbj_required_compile_time.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(TARGET) dbj-arena-hashmap-hashtrie.c

clean:
	rm -f $(TARGET) $(TARGET).exe
