# -fdefer-ts enables the defer keyword as specified by ISO/DIS TS 25755.
# It is opt-in even on GCC 15+, so it must be passed explicitly.
# When active, the compiler defines __STDC_DEFER_TS25755__, which strassen.c
# uses to skip the gcc_defer.h macro polyfill and rely on the native keyword
# instead.  Without this flag, gcc_defer.h is included as a fallback on any
# GCC version.

CC = G:\mingw64\bin\gcc.exe
CFLAGS = -std=gnu23 -O3
LDFLAGS = -lkernel32

bench.exe: strassen.c ubench.h
	$(CC) $(CFLAGS) -o $@ strassen.c $(LDFLAGS)

strassen_bench_comparator.exe: strassen_bench_comparator.c dbj_strassen_matmul.h ubench.h ../../dbj_nanobench/dbj_nanobench.h
	$(CC) $(CFLAGS) -o $@ strassen_bench_comparator.c $(LDFLAGS)

clean:
	-del /Q bench.exe strassen_bench_comparator.exe

.PHONY: clean
