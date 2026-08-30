#ifndef DBJ_THE_GAME_ENTITY_H
#define DBJ_THE_GAME_ENTITY_H

#include "world.h"

// The whole of what was a class hierarchy with a virtual execute().
// Static kinds have arms that do nothing -- written out rather than
// grouped, so -Wswitch still names them when a kind is added.
void entity_step(entity ent[static 1], float dt, input_state const in[static 1],
                 world w[static 1]);

bool entity_overlaps(entity const a[static 1], entity const b[static 1]);

#endif  // DBJ_THE_GAME_ENTITY_H
