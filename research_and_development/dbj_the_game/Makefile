# DBJ_CORELIB must be set in the environment and must point at the
# repo's corelib/ folder. It is not optional and there is no fallback:
# a wrong or missing corelib is a broken build, not a silent one.
ifndef DBJ_CORELIB
$(error DBJ_CORELIB is not set -- point it at <repo>/corelib)
endif

# Forward slashes throughout. make hands its recipes to sh where one is
# on PATH, and sh eats the backslashes out of a Windows path.
CORELIB   := $(subst \,/,$(DBJ_CORELIB))

CFLAGS    := -std=c23 -Wall -Wextra -Wswitch -Werror -I $(CORELIB)
# DBJ_BUILDS must be set in the environment and is where every folder's
# build output goes. Like DBJ_CORELIB it is not optional and has no
# fallback: the two say where the repo reads from and writes to, and a
# rule that holds for one holds for the other.
ifndef DBJ_BUILDS
$(error DBJ_BUILDS is not set -- point it at a build output folder)
endif
BUILD_DIR := $(subst \,/,$(DBJ_BUILDS))

SOURCES   := main.c world.c entity.c physics.c map.c input.c draw.c \
             dialogues.c dbj_configurator.c dbj_platform.c
# build_timestamp.inc is deliberately absent from this list. It is written
# by make.sh before every build, so naming it here would relink the test
# binary too -- and the simulation has no version string in it.
HEADERS   := main.h world.h entity.h physics.h map.h input.h draw.h \
             dbj_configurator.h dbj_platform.h \
             milestone_iteration.inc

# The simulation, with nothing that opens a window. The test binary links
# exactly this list -- if raylib ever reaches below draw.c, it stops linking.
SIM_SOURCES  := world.c entity.c physics.c map.c
TEST_SOURCES := tests/dbj_the_game_test.c $(SIM_SOURCES)
TAU_CFLAGS   := -I ../../third_party/tau

RAYLIB_DIR    := ../../third_party/raylib
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
# xcopy needs the /i to treat a missing destination as a directory rather
# than asking, and recipes cannot answer questions.
COPY_ASSETS := cmd /c xcopy /e /i /y /q assets $(subst /,\,$(BUILD_DIR))\assets
RAYLIB_LIBS := $(RAYLIB_DIR)/win64/libraylib.a -lopengl32 -lgdi32 -lwinmm
else
CC          := gcc-15
EXE         :=
MKDIR       := mkdir -p $(BUILD_DIR)
RM_F        := rm -f
native       = $1
COPY_ASSETS := cp -r assets $(BUILD_DIR)/
RAYLIB_LIBS := $(RAYLIB_DIR)/linux64/libraylib.a -lGL -lm -lpthread -ldl -lrt -lX11
endif

TARGET      := $(BUILD_DIR)/dbj_the_game$(EXE)
TEST_TARGET := $(BUILD_DIR)/dbj_the_game_test$(EXE)

.PHONY: all run test clean assets

all: $(TARGET) assets

# The program resolves assets against its own location, so they have to be
# there. Unconditional on purpose: an edited map or a redrawn sprite is not
# a dependency make can see through a directory.
assets: | $(BUILD_DIR)
	$(COPY_ASSETS)

$(BUILD_DIR):
	$(MKDIR)

# The wildcard is what keeps a bare `make` working on a fresh clone: the
# stamp is a dependency when it exists and simply is not one when it does
# not, which is exactly the __has_include fallback in main.h expressed to
# make. When make.sh rewrites it, this target relinks and the binary tells
# the truth about when it was built.
$(TARGET): $(SOURCES) $(HEADERS) $(wildcard build_timestamp.inc) \
           $(CORELIB)/dbj_defer.h $(CORELIB)/dbj_clintro.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(RAYLIB_CFLAGS) -o $(TARGET) $(SOURCES) $(RAYLIB_LIBS)

$(TEST_TARGET): $(TEST_SOURCES) $(HEADERS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(TAU_CFLAGS) -o $(TEST_TARGET) $(TEST_SOURCES)

# Assets sit beside the binary, so the working directory no longer matters:
# this runs the same from here, from the repo root, or from anywhere else.
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
