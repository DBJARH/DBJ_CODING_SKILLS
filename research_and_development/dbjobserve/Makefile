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

CFLAGS    := -std=c23 -Wall -Wextra -Wswitch -Werror -I $(CORELIB)
TARGET    := $(BUILD_DIR)/dbjobserve_smoketest

# libcurl is linked statically on both platforms, but sourced
# differently -- a MinGW .a cannot link on Linux, so only the Windows
# build is vendored. See general_design.md, "Portability".
#
# Windows/MinGW ships plain gcc.exe (gcc 15+); Linux distros still default
# `gcc` to an older version, so pin the versioned binary there.
ifeq ($(OS),Windows_NT)
CC       := gcc
MKDIR    := if not exist $(subst /,\,$(BUILD_DIR)) mkdir $(subst /,\,$(BUILD_DIR))

CURL_DIR := ../../third_party/libcurl
CURL_CFLAGS := -I $(CURL_DIR)/include -DCURL_STATICLIB
# libcurl.a is named by path, not via -lcurl: lib/ holds both libcurl.a
# and libcurl.dll.a, and the linker prefers the import lib -- which
# links cleanly, then dies at startup with 0xC0000135 (missing DLL).
# -DCURL_STATICLIB alone does not prevent this; it only controls
# dllimport in the headers. Static archives first, Windows system
# libraries last: secur32 (SSPI, InitSecurityInterfaceA) and iphlpapi
# (if_nametoindex) are the two that are easy to forget.
CURL_LIBS := $(CURL_DIR)/lib/libcurl.a -L $(CURL_DIR)/lib \
             -lssh2 -lssl -lcrypto \
             -lnghttp2 -lngtcp2_crypto_libressl -lngtcp2 -lnghttp3 \
             -lpsl -lbrotlidec -lbrotlicommon -lzstd -lz \
             -lbcrypt -lcrypt32 -lsecur32 -liphlpapi \
             -lws2_32 -lwldap32 -lwinmm -lgdi32 -ladvapi32 -luser32
LDFLAGS  := -static
else
CC       := gcc-15
MKDIR    := mkdir -p $(BUILD_DIR)

# Distro libcurl. Needs the -dev package (libcurl4-openssl-dev or
# libcurl-devel); `pkg-config --static` reports curl's own dependency
# chain, so it is not hardcoded here.
CURL_CFLAGS := $(shell pkg-config --cflags libcurl)
CURL_LIBS   := $(shell pkg-config --libs --static libcurl)
LDFLAGS  :=
endif

.PHONY: all clean

all: $(TARGET)

$(BUILD_DIR):
	$(MKDIR)

$(TARGET): dbjobserve_smoketest.c ../../third_party/inifile/inifile.h \
           $(CORELIB)/dbj_defer.h $(CORELIB)/dbj_result.h \
           $(CORELIB)/dbj_simple_log.h $(CORELIB)/dbj_clintro.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CURL_CFLAGS) $(LDFLAGS) -o $(TARGET) \
	      dbjobserve_smoketest.c $(CURL_LIBS)

clean:
	rm -f $(TARGET) $(TARGET).exe
