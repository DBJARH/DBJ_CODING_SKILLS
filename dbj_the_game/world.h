// dbj_the_game -- simulation state. No raylib here, on purpose:
// everything below draw.c is plain C on plain data, so a recorded
// input array can replay a whole game with no window open.
#ifndef DBJ_THE_GAME_WORLD_H
#define DBJ_THE_GAME_WORLD_H

#include <stdbool.h>

#define CELL 40.0f  // map grid cell, pixels -- the ancestor's unit

// Arena capacities. Derived, not chosen -- see docs/implementation.md,
// "Derived constants". Editing ENEMY_CAP without ENEMIES_MAX is the bug
// the comment in world.c exists to prevent.
#define PLAYERS_MAX      2
#define ENEMY_CAP       15  // alive at once, not a level total
#define ENEMIES_MAX     (ENEMY_CAP + 1)
#define PROJECTILES_MAX 64
#define OBSTACLES_MAX  320

typedef struct {
	float x, y;
} vec2;

// The tag. No default case is ever written over this enum, so -Wswitch
// -Werror turns a forgotten kind into a build error.
typedef enum {
	ENTITY_PLAYER,
	ENTITY_WARRIOR,
	ENTITY_PROJECTILE,
	ENTITY_PLATFORM,
	ENTITY_SPIKE,
	ENTITY_FIRE,
} entity_kind;

// Player sub-state: what was a std::map<string,State*> in the ancestor.
typedef enum {
	PLAYER_REST,
	PLAYER_WALK,
	PLAYER_JUMP,
} player_motion;

// One struct for every kind. Fields unused by a kind simply sit unused;
// every field is named for what it holds -- no general-purpose slots
// whose meaning depends on the tag.
typedef struct {
	entity_kind kind;
	vec2 pos;
	vec2 vel;
	vec2 size;
	int life;
	bool facing_right;
	bool grounded;
	player_motion motion;
	float fire_cooldown;
	float hurt_flash;
	float expiry;
	float anim_time;    // seconds in the current motion; drives the walk cycle
} entity;

// Storage is inline and caller-owned. `world w = {0}` is a valid empty
// world: every count is 0 and slots past a count are never read.
// RULE: no field may join this struct whose zero value is invalid.
typedef struct {
	entity players[PLAYERS_MAX];
	entity enemies[ENEMIES_MAX];
	entity projectiles[PROJECTILES_MAX];
	entity obstacles[OBSTACLES_MAX];
	int player_count;
	int enemy_count;
	int projectile_count;
	int obstacle_count;
	vec2 spawn_points[OBSTACLES_MAX];
	int spawn_count;
	int spawn_cursor;   // round-robin over spawn_points
	float respawn_timer;
	bool player_dead;
} world;

typedef struct {
	bool left, right, jump, attack;
} input_state;

// Note the parameter order: a size is declared before the array it
// bounds, because a size expression may only name parameters already in
// scope. This inverts the usual (buffer, size) habit throughout.
void world_step(world w[static 1], float dt, input_state const in[static 1]);
void world_respawn(world w[static 1], float dt);
entity *world_spawn(world w[static 1], entity_kind kind, vec2 at);

#endif  // DBJ_THE_GAME_WORLD_H
