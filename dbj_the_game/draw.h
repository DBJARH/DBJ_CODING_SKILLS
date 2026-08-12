#ifndef DBJ_THE_GAME_DRAW_H
#define DBJ_THE_GAME_DRAW_H

#include "dbj_configurator.h"
#include "world.h"

#define WINDOW_WIDTH  840
#define WINDOW_HEIGHT 480

// Textures are the only resource the renderer owns. Load after InitWindow,
// unload before CloseWindow -- a GPU handle outliving its context is the one
// ordering mistake raylib will not warn about.
//
// Returns false when any sheet could not be resolved or read. raylib answers
// a missing file with a zeroed texture and a log line, which draws nothing
// and stops nothing -- so the check belongs here, where the path is known.
bool draw_load_art(dbj_configurator const cfg[static 1]);
void draw_unload_art(void);
void draw_world(world const w[static 1]);

#endif  // DBJ_THE_GAME_DRAW_H
