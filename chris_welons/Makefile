# gnu23, not c2x: toplevel/dbj_defer.h needs GNU nested functions.
# -Wswitch-enum, not just -Wswitch: -Wswitch goes silent the moment a
# `default` appears, which is exactly the mistake CLAUDE.md forbids.
# -Wswitch-enum names every unlisted enum value regardless.
CFLAGS    := -std=gnu23 -Wall -Wextra -Wswitch-enum -Werror -I ../toplevel
# DBJ_BUILDS, if set in the environment, is where every folder's build
# output goes; falls back to ../builds (repo root) if unset.
BUILD_DIR := $(if $(DBJ_BUILDS),$(DBJ_BUILDS),../builds)
TARGET       := $(BUILD_DIR)/dbj_arena_hashmap_hashtrie
BENCH_TARGET := $(BUILD_DIR)/wellons_benchmark

# Windows/MinGW ships plain gcc.exe (gcc 15+); Linux distros still default
# `gcc` to an older version, so pin the versioned binary there.
ifeq ($(OS),Windows_NT)
CC      := gcc
MKDIR   := if not exist $(subst /,\,$(BUILD_DIR)) mkdir $(subst /,\,$(BUILD_DIR))
else
CC      := gcc-15
MKDIR   := mkdir -p $(BUILD_DIR)
endif

.PHONY: all clean bench

all: $(TARGET) $(BENCH_TARGET)

$(BUILD_DIR):
	$(MKDIR)

$(TARGET): dbj-arena-hashmap-hashtrie.c \
           ../toplevel/dbj_str_slice.h \
           ../toplevel/dbj_clintro.h \
           ../toplevel/dbj_defer.h \
           ../toplevel/dbj_required_compile_time.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(TARGET) dbj-arena-hashmap-hashtrie.c

# Wellons' own code benchmarked, by #including his file with
# WELLONS_AS_LIB to suppress its main(). -O2: timing an unoptimised
# build measures something nobody ships.
$(BENCH_TARGET): wellons_benchmark.c \
           wellons-corelib.c \
           ../toplevel/dbj_nanobench.h \
           ../toplevel/dbj_clintro.h \
           ../toplevel/dbj_required_compile_time.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -O2 -o $(BENCH_TARGET) wellons_benchmark.c

bench: $(BENCH_TARGET)
	$(BENCH_TARGET)

clean:
	rm -f $(TARGET) $(TARGET).exe $(BENCH_TARGET) $(BENCH_TARGET).exe
