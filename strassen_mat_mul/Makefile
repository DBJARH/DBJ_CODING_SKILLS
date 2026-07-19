# -fdefer-ts enables the defer keyword as specified by ISO/DIS TS 25755.
# It is opt-in even on GCC 15+, so it must be passed explicitly.
# When active, the compiler defines __STDC_DEFER_TS25755__, which strassen.c
# uses to skip the <dbj_defer.h> macro polyfill and rely on the native
# keyword instead.  Without this flag, <dbj_defer.h> is included as a
# fallback on any GCC version.  gcc_defer.h is deprecated; kept for reference
# only.  -I../toplevel resolves toplevel headers as <dbj_*.h>.

CC = G:\mingw64\bin\gcc.exe
CFLAGS = -std=gnu23 -O3 -I../toplevel

bench.exe: strassen.c ../toplevel/dbj_defer.h ../dbj_nanobench/dbj_nanobench.h
	$(CC) $(CFLAGS) -o $@ strassen.c

strassen_bench_comparator.exe: strassen_bench_comparator.c dbj_strassen_matmul.h ../dbj_nanobench/dbj_nanobench.h
	$(CC) $(CFLAGS) -o $@ strassen_bench_comparator.c

soa_aso_comparator.exe: soa_aso_comparator.c ../toplevel/dbj_defer.h ../dbj_nanobench/dbj_nanobench.h
	$(CC) $(CFLAGS) -o $@ soa_aso_comparator.c

clean:
	-del /Q bench.exe strassen_bench_comparator.exe soa_aso_comparator.exe

.PHONY: clean
