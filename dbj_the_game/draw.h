#ifndef DBJ_THE_GAME_DRAW_H
#define DBJ_THE_GAME_DRAW_H

#include "world.h"

#define WINDOW_WIDTH  840
#define WINDOW_HEIGHT 480

// Textures are the only resource the renderer owns. Load after InitWindow,
// unload before CloseWindow -- a GPU handle outliving its context is the one
// ordering mistake raylib will not warn about.
void draw_load_art(void);
void draw_unload_art(void);
void draw_world(world const w[static 1]);

#endif  // DBJ_THE_GAME_DRAW_H
