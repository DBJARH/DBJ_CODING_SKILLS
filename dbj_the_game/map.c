#include "map.h"

#include <stdio.h>

#include <dbj_defer.h>

#define MAP_LINE_MAX 512

bool map_load(world w[static 1], char const path[static 1])
{
	FILE *input = fopen(path, "r");
	if (!input) return false;
	defer { fclose(input); }

	bool ok = true;
	char line[MAP_LINE_MAX];
	int row = 0;

	while (fgets(line, sizeof line, input)) {
		for (int col = 0; line[col] && line[col] != '\n'; ++col) {
			vec2 at = {(float)col * CELL, (float)row * CELL};
			switch (line[col]) {
			case '1':
				if (!world_spawn(w, ENTITY_PLATFORM, at)) ok = false;
				break;
			case '2':
				if (!world_spawn(w, ENTITY_SPIKE, at)) ok = false;
				break;
			case '3':
				if (!world_spawn(w, ENTITY_FIRE, at)) ok = false;
				break;
			case '&':
				if (w->spawn_count < OBSTACLES_MAX)
					w->spawn_points[w->spawn_count++] = at;
				break;
			case 'P':
				if (!world_spawn(w, ENTITY_PLAYER, at)) ok = false;
				break;
			default:
				break;   // '*' and anything else is empty space
			}
		}
		++row;
	}
	return ok;
}
