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

# gnu23, not c2x: corelib/dbj_defer.h needs GNU nested functions.
# -Wswitch-enum, not just -Wswitch: -Wswitch goes silent the moment a
# `default` appears, which is exactly the mistake CLAUDE.md forbids.
# -Wswitch-enum names every unlisted enum value regardless.
CFLAGS    := -std=gnu23 -Wall -Wextra -Wswitch-enum -Werror -I $(CORELIB) \
             -I ../../third_party/dbj_ubenchtest
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
           $(CORELIB)/dbj_str_slice.h \
           $(CORELIB)/dbj_clintro.h \
           $(CORELIB)/dbj_defer.h \
           $(CORELIB)/dbj_required_compile_time.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(TARGET) dbj-arena-hashmap-hashtrie.c

# Wellons' own code benchmarked, by #including his file with
# WELLONS_AS_LIB to suppress its main(). -O2: timing an unoptimised
# build measures something nobody ships.
$(BENCH_TARGET): wellons_benchmark.c \
           wellons-corelib.c \
           ../../third_party/dbj_ubenchtest/dbj_ubenchtest.h \
           $(CORELIB)/dbj_clintro.h \
           $(CORELIB)/dbj_required_compile_time.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -O2 -o $(BENCH_TARGET) wellons_benchmark.c

bench: $(BENCH_TARGET)
	$(BENCH_TARGET)

clean:
	rm -f $(TARGET) $(TARGET).exe $(BENCH_TARGET) $(BENCH_TARGET).exe
