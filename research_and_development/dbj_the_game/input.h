#ifndef DBJ_THE_GAME_INPUT_H
#define DBJ_THE_GAME_INPUT_H

#include "world.h"

// Only this and draw.c see raylib. Everything else takes input_state,
// which is a plain struct -- so a recorded array of them replays a game
// with no window, which is what makes the simulation testable.
input_state input_poll(void);

#endif  // DBJ_THE_GAME_INPUT_H
