#include "draw.h"

#include <raylib.h>

// Placeholder art: flat colours, no texture files. The design rules out
// upstream assets, so nothing here loads from disk.
static Color kind_colour(entity const ent[static 1])
{
	switch (ent->kind) {
	case ENTITY_PLAYER:     return ent->hurt_flash > 0.0f ? RED : SKYBLUE;
	case ENTITY_WARRIOR:    return ent->hurt_flash > 0.0f ? WHITE : MAROON;
	case ENTITY_PROJECTILE: return GOLD;
	case ENTITY_PLATFORM:   return DARKGRAY;
	case ENTITY_SPIKE:      return ORANGE;
	case ENTITY_FIRE:       return RED;
	}
	return MAGENTA;
}

static void draw_arena(entity const arena[static 1], int count)
{
	for (int i = 0; i < count; ++i) {
		entity const *ent = &arena[i];
		DrawRectangle((int)ent->pos.x, (int)ent->pos.y,
		              (int)ent->size.x, (int)ent->size.y,
		              kind_colour(ent));
	}
}

// The camera is derived, not stored. world holds no camera: a zeroed
// Camera2D has zoom == 0 and renders nothing, and raylib in world.h
// would break testing with no window.
static Camera2D follow_camera(world const w[static 1])
{
	float target_x = w->player_count > 0 ? w->players[0].pos.x : 0.0f;
	return (Camera2D){
		.target = {target_x, (float)WINDOW_HEIGHT / 2.0f},
		.offset = {(float)WINDOW_WIDTH / 2.0f, (float)WINDOW_HEIGHT / 2.0f},
		.rotation = 0.0f,
		.zoom = 1.0f,
	};
}

void draw_world(world const w[static 1])
{
	BeginDrawing();
	ClearBackground((Color){20, 18, 30, 255});

	BeginMode2D(follow_camera(w));
	if (w->obstacle_count > 0)   draw_arena(w->obstacles, w->obstacle_count);
	if (w->enemy_count > 0)      draw_arena(w->enemies, w->enemy_count);
	if (w->projectile_count > 0) draw_arena(w->projectiles, w->projectile_count);
	if (w->player_count > 0)     draw_arena(w->players, w->player_count);
	EndMode2D();

	if (w->player_count > 0)
		DrawText(TextFormat("life %d", w->players[0].life), 10, 10, 20, RAYWHITE);
	else
		DrawText("dead", 10, 10, 20, RED);
	DrawText(TextFormat("enemies %d", w->enemy_count), 10, 34, 20, RAYWHITE);
	DrawFPS(WINDOW_WIDTH - 90, 10);

	EndDrawing();
}
