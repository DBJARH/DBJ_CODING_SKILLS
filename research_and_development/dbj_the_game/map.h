#ifndef DBJ_THE_GAME_MAP_H
#define DBJ_THE_GAME_MAP_H

#include "world.h"

// Grid legend, one char per cell:
//   '1' platform   '2' spike   '3' fire   '&' enemy spawn point
//   'P' player start   '*' empty
// Returns false if the file cannot be read or the arena filled up.
bool map_load(world w[static 1], char const path[static 1]);

#endif  // DBJ_THE_GAME_MAP_H
