# gnu23, not c2x: toplevel/dbj_defer.h needs GNU nested functions.
# -Wswitch-enum, not just -Wswitch: -Wswitch goes silent the moment a
# `default` appears, which is exactly the mistake CLAUDE.md forbids.
# -Wswitch-enum names every unlisted enum value regardless.
CFLAGS    := -std=gnu23 -Wall -Wextra -Wswitch-enum -Werror -I ../toplevel
# DBJ_BUILDS, if set in the environment, is where every folder's build
# output goes; falls back to ../builds (repo root) if unset.
BUILD_DIR := $(if $(DBJ_BUILDS),$(DBJ_BUILDS),../builds)
SLICE_TARGET := $(BUILD_DIR)/dbj_arena_hashmap_hashtrie
STR_TARGET   := $(BUILD_DIR)/dbj_str_4_welons

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

all: $(SLICE_TARGET) $(STR_TARGET)

$(BUILD_DIR):
	$(MKDIR)

# the dbj_str_slice variant: keys are views into the arena
$(SLICE_TARGET): dbj-arena-hashmap-hashtrie.c \
           ../toplevel/dbj_str_slice.h \
           ../toplevel/dbj_clintro.h \
           ../toplevel/dbj_defer.h \
           ../toplevel/dbj_required_compile_time.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(SLICE_TARGET) dbj-arena-hashmap-hashtrie.c

# the dbj_str variant: keys own their bytes, nothing points anywhere.
# The map itself lives in ../dbj_hashmap/ -- this is only a user of it.
$(STR_TARGET): dbj-str-4-welons.c \
           ../dbj_hashmap/dbj_hashmap.h \
           ../dbj_hashmap/dbj_arena.h \
           ../dbj_hashmap/dbj_hash_string.h \
           ../toplevel/dbj_str.h \
           ../toplevel/dbj_result.h \
           ../toplevel/dbj_nanobench.h \
           ../toplevel/dbj_clintro.h \
           ../toplevel/dbj_defer.h \
           ../toplevel/dbj_required_compile_time.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -O2 -I ../dbj_hashmap -o $(STR_TARGET) dbj-str-4-welons.c

clean:
	rm -f $(SLICE_TARGET) $(SLICE_TARGET).exe $(STR_TARGET) $(STR_TARGET).exe
