#include "input.h"

#include <raylib.h>

input_state input_poll(void)
{
	return (input_state){
		.left   = IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A),
		.right  = IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D),
		.jump   = IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W) ||
		          IsKeyDown(KEY_SPACE),
		.attack = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_F),
	};
}
