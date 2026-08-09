#include "draw.h"

#include <raylib.h>

// Sprite sheets from the C++ ancestor in lineage/tec_drakula, MIT
// (© Pedro Foresti Leão), used with the author's written permission.
// Every rect below is the ancestor's own, not measured by eye:
// Player.h, Warrior.h, Obstacle.h, Dracula.h.
#define WALK_FRAME_TIME 0.1f
#define WALK_FRAMES     8

static Texture2D tex_player;
static Texture2D tex_warrior;
static Texture2D tex_knife;
static Texture2D tex_obstacle;
static Texture2D tex_fire;

void draw_load_art(void)
{
	tex_player   = LoadTexture("assets/tileset_player.png");
	tex_warrior  = LoadTexture("assets/warrior.png");
	tex_knife    = LoadTexture("assets/knife.png");
	tex_obstacle = LoadTexture("assets/obstacles.png");
	tex_fire     = LoadTexture("assets/fire.png");
}

void draw_unload_art(void)
{
	UnloadTexture(tex_player);
	UnloadTexture(tex_warrior);
	UnloadTexture(tex_knife);
	UnloadTexture(tex_obstacle);
	UnloadTexture(tex_fire);
}

// Which sheet a kind draws from. A kind with no art returns a zeroed
// texture, whose id is 0 -- the caller falls back to a flat rectangle.
static Texture2D kind_sheet(entity_kind kind)
{
	switch (kind) {
	case ENTITY_PLAYER:     return tex_player;
	case ENTITY_WARRIOR:    return tex_warrior;
	case ENTITY_PROJECTILE: return tex_knife;
	case ENTITY_PLATFORM:   return tex_obstacle;
	case ENTITY_SPIKE:      return tex_obstacle;
	case ENTITY_FIRE:       return tex_fire;
	}
	return (Texture2D){0};
}

// The player's sheet is the only animated one: rest and jump are single
// frames, walk is eight stepped by the entity's own clock.
static Rectangle player_frame(entity const ent[static 1])
{
	switch (ent->motion) {
	case PLAYER_REST:
		return (Rectangle){0.0f, 0.0f, 21.0f, 43.0f};
	case PLAYER_JUMP:
		return (Rectangle){0.0f, 133.0f, 26.0f, 44.0f};
	case PLAYER_WALK: {
		int step = (int)(ent->anim_time / WALK_FRAME_TIME) % WALK_FRAMES;
		return (Rectangle){36.0f * (float)step, 44.0f, 33.0f, 44.0f};
	}
	}
	return (Rectangle){0.0f, 0.0f, 21.0f, 43.0f};
}

static Rectangle kind_frame(entity const ent[static 1])
{
	switch (ent->kind) {
	case ENTITY_PLAYER:     return player_frame(ent);
	case ENTITY_WARRIOR:    return (Rectangle){0.0f, 0.0f, 27.0f, 41.0f};
	case ENTITY_PROJECTILE: return (Rectangle){0.0f, 0.0f, 13.0f, 4.0f};
	// obstacles.png is a 6-tile strip; the ancestor indexes it by
	// obstacleId (Obstacle::updateRect), castle platform 0, spike 2.
	// Here the tag supplies the index, so no entity field is needed.
	case ENTITY_PLATFORM:   return (Rectangle){0.0f * 40.0f, 0.0f, 40.0f, 40.0f};
	case ENTITY_SPIKE:      return (Rectangle){2.0f * 40.0f, 0.0f, 40.0f, 40.0f};
	case ENTITY_FIRE:       return (Rectangle){0.0f, 0.0f, 16.0f, 11.0f};
	}
	return (Rectangle){0.0f, 0.0f, 40.0f, 40.0f};
}

// Colour when a kind has no sheet, and the tint when it has one.
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

static void draw_entity(entity const ent[static 1])
{
	Texture2D sheet = kind_sheet(ent->kind);
	if (sheet.id == 0) {
		DrawRectangle((int)ent->pos.x, (int)ent->pos.y,
		              (int)ent->size.x, (int)ent->size.y,
		              kind_colour(ent));
		return;
	}

	Rectangle src = kind_frame(ent);
	// A negative source width mirrors the frame -- the ancestor's own
	// idiom (WARRIOR_LEFT_RECT, REST_L), and raylib reads it the same way.
	if (!ent->facing_right) src.width = -src.width;

	Rectangle dst = {ent->pos.x, ent->pos.y, ent->size.x, ent->size.y};
	// Hit entities flash white; untouched ones draw at their own colours.
	Color tint = ent->hurt_flash > 0.0f ? (Color){255, 120, 120, 255} : WHITE;
	DrawTexturePro(sheet, src, dst, (Vector2){0.0f, 0.0f}, 0.0f, tint);
}

static void draw_arena(entity const arena[static 1], int count)
{
	for (int i = 0; i < count; ++i)
		draw_entity(&arena[i]);
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
