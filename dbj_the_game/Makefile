CFLAGS    := -std=c23 -Wall -Wextra -Wswitch -Werror -I ../toplevel
# DBJ_BUILDS, if set in the environment, is where every folder's build
# output goes; falls back to ../builds (repo root) if unset.
BUILD_DIR := $(if $(DBJ_BUILDS),$(DBJ_BUILDS),../builds)

SOURCES   := main.c world.c entity.c physics.c map.c input.c draw.c
# build_timestamp.inc is deliberately absent from this list. It is written
# by make.sh before every build, so naming it here would relink the test
# binary too -- and the simulation has no version string in it.
HEADERS   := main.h world.h entity.h physics.h map.h input.h draw.h \
             milestone_iteration.inc

# The simulation, with nothing that opens a window. The test binary links
# exactly this list -- if raylib ever reaches below draw.c, it stops linking.
SIM_SOURCES  := world.c entity.c physics.c map.c
TEST_SOURCES := tests/dbj_the_game_test.c $(SIM_SOURCES)
TAU_CFLAGS   := -I ../third_party/tau

RAYLIB_DIR    := ../third_party/raylib
RAYLIB_CFLAGS := -I $(RAYLIB_DIR)/include

# Windows/MinGW ships plain gcc.exe (gcc 15+); Linux distros still default
# `gcc` to an older version, so pin the versioned binary there.
#
# EXE matters to make, not just to the shell: MinGW appends .exe to any -o
# without an extension, so a target named without it never appears on disk
# and every build relinks from scratch.
#
# raylib is named by path, never -lraylib: if an import library ever lands
# beside the static one, the linker prefers it -- the build succeeds and the
# binary dies at startup on a missing DLL. Platform libraries come *after*
# the archive, or ld reports undefined references from code plainly present.
ifeq ($(OS),Windows_NT)
CC          := gcc
EXE         := .exe
MKDIR       := if not exist $(subst /,\,$(BUILD_DIR)) mkdir $(subst /,\,$(BUILD_DIR))
# `cmd /c` is not decoration: del is a cmd builtin, and make spawns recipe
# lines with CreateProcess, which finds no del.exe to run.
RM_F        := cmd /c del /q
native       = $(subst /,\,$1)
RAYLIB_LIBS := $(RAYLIB_DIR)/win64/libraylib.a -lopengl32 -lgdi32 -lwinmm
else
CC          := gcc-15
EXE         :=
MKDIR       := mkdir -p $(BUILD_DIR)
RM_F        := rm -f
native       = $1
RAYLIB_LIBS := $(RAYLIB_DIR)/linux64/libraylib.a -lGL -lm -lpthread -ldl -lrt -lX11
endif

TARGET      := $(BUILD_DIR)/dbj_the_game$(EXE)
TEST_TARGET := $(BUILD_DIR)/dbj_the_game_test$(EXE)

.PHONY: all run test clean

all: $(TARGET)

$(BUILD_DIR):
	$(MKDIR)

# The wildcard is what keeps a bare `make` working on a fresh clone: the
# stamp is a dependency when it exists and simply is not one when it does
# not, which is exactly the __has_include fallback in main.h expressed to
# make. When make.sh rewrites it, this target relinks and the binary tells
# the truth about when it was built.
$(TARGET): $(SOURCES) $(HEADERS) $(wildcard build_timestamp.inc) \
           ../toplevel/dbj_defer.h ../toplevel/dbj_clintro.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(RAYLIB_CFLAGS) -o $(TARGET) $(SOURCES) $(RAYLIB_LIBS)

$(TEST_TARGET): $(TEST_SOURCES) $(HEADERS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(TAU_CFLAGS) -o $(TEST_TARGET) $(TEST_SOURCES)

# Assets are resolved relative to the working directory, so run from here.
run: all
	$(TARGET)

# The suite builds its own world in code, so it needs no working directory.
test: $(TEST_TARGET)
	$(TEST_TARGET)

# Leading '-': cmd.exe's del fails loudly on an already-absent file, and a
# clean that has nothing to clean has succeeded.
#
# build_timestamp.inc belongs here for the same reason the binaries do: it
# is generated. make.sh writes it before every invocation including this
# one, so `make.sh clean` writes it and then this removes it -- without the
# line below, the one generated file in the folder is the one that survives
# the command whose whole job is removing generated files.
clean:
	-$(RM_F) $(call native,$(TARGET)) $(call native,$(TEST_TARGET))
	-$(RM_F) $(call native,build_timestamp.inc)
