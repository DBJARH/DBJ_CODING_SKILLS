// DBJ_THE_GAME -- iteration one.
// Milestone 1: one stage, one player, warriors only, no boss, no end
// state. The stage refills forever; that is a stated choice, see
// docs/design.md "Open decisions".
#include <raylib.h>

#include <dbj_clintro.h>
#include <dbj_defer.h>

#include <stdio.h>
#include <string.h>

#include "draw.h"
#include "input.h"
#include "main.h"
#include "map.h"
#include "world.h"

#define MAX_FRAME_DT (1.0f / 30.0f)

// --version answers and leaves. Asked where a build sits on the ladder, the
// program must not need a window, a stage file or a graphics driver to say.
static bool wants_version(int argc, char *argv[static argc + 1])
{
	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "--version") == 0) return true;
	}
	return false;
}

int main(int argc, char *argv[static argc + 1])
{
	if (wants_version(argc, argv)) {
		puts(DBJ_THE_GAME_VERSION);
		return 0;
	}

	dbj_clintro("DBJ_THE_GAME", DBJ_THE_GAME_VERSION);

	// A zeroed world is a valid empty world -- every count is 0.
	world game = {0};
	if (!map_load(&game, "assets/castle.txt")) {
		TraceLog(LOG_ERROR, "map load failed: assets/castle.txt");
		return 1;
	}
	if (game.player_count == 0) {
		TraceLog(LOG_ERROR, "map has no 'P' player start");
		return 1;
	}

	InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "DBJ_THE_GAME");
	defer { CloseWindow(); }
	SetTargetFPS(60);

	// Textures need a GL context, so this cannot move above InitWindow --
	// and their handles must not outlive it either. Defers run in reverse,
	// so declaring this one second unloads the art before the window goes:
	// the ordering is the language's to keep, not the reader's.
	draw_load_art();
	defer { draw_unload_art(); }

	while (!WindowShouldClose()) {
		input_state now = input_poll();

		// Clamp dt: a long stall must not tunnel entities through
		// platforms, since collision here is discrete.
		float dt = GetFrameTime();
		if (dt > MAX_FRAME_DT) dt = MAX_FRAME_DT;

		world_step(&game, dt, &now);
		draw_world(&game);
	}

	return 0;
}
