# -fdefer-ts enables the defer keyword as specified by ISO/DIS TS 25755.
# It is opt-in even on GCC 15+, so it must be passed explicitly.
# When active, the compiler defines __STDC_DEFER_TS25755__, which strassen.c
# uses to skip the <dbj_defer.h> macro polyfill and rely on the native
# keyword instead.  Without this flag, <dbj_defer.h> is included as a
# fallback on any GCC version.  gcc_defer.h is deprecated; kept for reference
# only.  -I../toplevel resolves toplevel headers as <dbj_*.h>.

CC = G:\mingw64\bin\gcc.exe
CFLAGS = -std=gnu23 -O3 -I../toplevel

# DBJ_BUILDS, if set in the environment, is where every folder's build
# output goes; falls back to ../builds (repo root) if unset.
BUILD_DIR := $(if $(DBJ_BUILDS),$(DBJ_BUILDS),../builds)

ifeq ($(OS),Windows_NT)
MKDIR := if not exist $(subst /,\,$(BUILD_DIR)) mkdir $(subst /,\,$(BUILD_DIR))
else
MKDIR := mkdir -p $(BUILD_DIR)
endif

.PHONY: all clean

all: $(BUILD_DIR)/bench.exe $(BUILD_DIR)/strassen_bench_comparator.exe $(BUILD_DIR)/soa_aso_comparator.exe $(BUILD_DIR)/dbj_soa_aso.exe

$(BUILD_DIR):
	$(MKDIR)

$(BUILD_DIR)/bench.exe: strassen.c ../toplevel/dbj_defer.h ../dbj_nanobench/dbj_nanobench.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ strassen.c

$(BUILD_DIR)/strassen_bench_comparator.exe: strassen_bench_comparator.c dbj_strassen_matmul.h ../dbj_nanobench/dbj_nanobench.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ strassen_bench_comparator.c

$(BUILD_DIR)/soa_aso_comparator.exe: soa_aso_comparator.c dbj_strassen_matmul.h ../toplevel/dbj_defer.h ../dbj_nanobench/dbj_nanobench.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ soa_aso_comparator.c

$(BUILD_DIR)/dbj_soa_aso.exe: dbj_soa_aso.c dbj_strassen_matmul.h ../toplevel/dbj_defer.h ../toplevel/dbj_macros.h ../toplevel/dbj_clintro.h ../dbj_nanobench/dbj_nanobench.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ dbj_soa_aso.c

clean:
	-del /Q $(subst /,\,$(BUILD_DIR)\bench.exe) $(subst /,\,$(BUILD_DIR)\strassen_bench_comparator.exe) $(subst /,\,$(BUILD_DIR)\soa_aso_comparator.exe) $(subst /,\,$(BUILD_DIR)\dbj_soa_aso.exe)
