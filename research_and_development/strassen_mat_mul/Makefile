# -fdefer-ts enables the defer keyword as specified by ISO/DIS TS 25755.
# It is opt-in even on GCC 15+, so it must be passed explicitly.
# When active, the compiler defines __STDC_DEFER_TS25755__, which strassen.c
# uses to skip the <dbj_defer.h> macro polyfill and rely on the native
# keyword instead.  Without this flag, <dbj_defer.h> is included as a
# fallback on any GCC version.  gcc_defer.h is deprecated; kept for reference
# only.  -I $(CORELIB) resolves corelib headers as <dbj_*.h>.

# DBJ_CORELIB must be set in the environment and must point at the
# repo's corelib/ folder. It is not optional and there is no fallback:
# a wrong or missing corelib is a broken build, not a silent one.
ifndef DBJ_CORELIB
$(error DBJ_CORELIB is not set -- point it at <repo>/corelib)
endif

# plain gcc from PATH -- the old G:\mingw64 path no longer exists; the
# toolchain lives at G:\mingw, which is on PATH already
CC = gcc

# Forward slashes throughout. make hands its recipes to sh where one is
# on PATH, and sh eats the backslashes out of a Windows path.
CORELIB   := $(subst \,/,$(DBJ_CORELIB))
# DBJ_BUILDS, if set in the environment, is where every folder's build
# output goes; falls back to ../../builds (repo root) if unset.
BUILD_DIR := $(subst \,/,$(if $(DBJ_BUILDS),$(DBJ_BUILDS),../../builds))

CFLAGS = -std=gnu23 -O3 -I $(CORELIB) -I ../../third_party/dbj_ubenchtest

ifeq ($(OS),Windows_NT)
MKDIR := if not exist $(subst /,\,$(BUILD_DIR)) mkdir $(subst /,\,$(BUILD_DIR))
else
MKDIR := mkdir -p $(BUILD_DIR)
endif

.PHONY: all clean

all: $(BUILD_DIR)/bench.exe $(BUILD_DIR)/strassen_bench_comparator.exe $(BUILD_DIR)/soa_aso_comparator.exe $(BUILD_DIR)/dbj_soa_aso.exe

$(BUILD_DIR):
	$(MKDIR)

$(BUILD_DIR)/bench.exe: strassen.c $(CORELIB)/dbj_defer.h ../../third_party/dbj_ubenchtest/dbj_ubenchtest.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ strassen.c

$(BUILD_DIR)/strassen_bench_comparator.exe: strassen_bench_comparator.c dbj_strassen_matmul.h ../../third_party/dbj_ubenchtest/dbj_ubenchtest.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ strassen_bench_comparator.c

$(BUILD_DIR)/soa_aso_comparator.exe: soa_aso_comparator.c dbj_strassen_matmul.h $(CORELIB)/dbj_defer.h ../../third_party/dbj_ubenchtest/dbj_ubenchtest.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ soa_aso_comparator.c

$(BUILD_DIR)/dbj_soa_aso.exe: dbj_soa_aso.c dbj_strassen_matmul.h $(CORELIB)/dbj_defer.h $(CORELIB)/dbj_macros.h $(CORELIB)/dbj_clintro.h ../../third_party/dbj_ubenchtest/dbj_ubenchtest.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ dbj_soa_aso.c

clean:
	-del /Q $(subst /,\,$(BUILD_DIR)\bench.exe) $(subst /,\,$(BUILD_DIR)\strassen_bench_comparator.exe) $(subst /,\,$(BUILD_DIR)\soa_aso_comparator.exe) $(subst /,\,$(BUILD_DIR)\dbj_soa_aso.exe)
