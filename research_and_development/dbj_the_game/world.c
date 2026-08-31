#include "world.h"
#include "entity.h"
#include "physics.h"

#define RESPAWN_INTERVAL 2.0f

// Slot dimensions per kind. Sizes are the ancestor's, in map cells.
static vec2 kind_size(entity_kind kind)
{
	switch (kind) {
	case ENTITY_PLAYER:     return (vec2){21.0f, 43.0f};
	case ENTITY_WARRIOR:    return (vec2){30.0f, 40.0f};
	case ENTITY_PROJECTILE: return (vec2){13.0f, 4.0f};
	case ENTITY_PLATFORM:   return (vec2){CELL, CELL};
	case ENTITY_SPIKE:      return (vec2){CELL, CELL};
	case ENTITY_FIRE:       return (vec2){CELL, CELL};
	}
	// Not dead code: -Wreturn-type does not accept an exhaustive switch as
	// an exit, and -Werror makes that fatal. -Wswitch still catches a
	// forgotten kind above; this line only keeps the function well-formed.
	return (vec2){0};
}

static int kind_life(entity_kind kind)
{
	switch (kind) {
	case ENTITY_PLAYER:     return 20;
	case ENTITY_WARRIOR:    return 15;
	case ENTITY_PROJECTILE: return 1;
	case ENTITY_PLATFORM:   return 1;
	case ENTITY_SPIKE:      return 1;
	case ENTITY_FIRE:       return 1;
	}
	return 1;   // same as kind_size: required by -Wreturn-type, not dead
}

// Claim a slot, or refuse. Refusal is null and the caller must cope --
// a silently dropped spawn is the bug a fixed arena would otherwise
// trade a leak for.
entity *world_spawn(world w[static 1], entity_kind kind, vec2 at)
{
	entity *slot = nullptr;
	switch (kind) {
	case ENTITY_PLAYER:
		if (w->player_count >= PLAYERS_MAX) return nullptr;
		slot = &w->players[w->player_count++];
		break;
	case ENTITY_WARRIOR:
		if (w->enemy_count >= ENEMIES_MAX) return nullptr;
		slot = &w->enemies[w->enemy_count++];
		break;
	case ENTITY_PROJECTILE:
		if (w->projectile_count >= PROJECTILES_MAX) return nullptr;
		slot = &w->projectiles[w->projectile_count++];
		break;
	case ENTITY_PLATFORM:
	case ENTITY_SPIKE:
	case ENTITY_FIRE:
		if (w->obstacle_count >= OBSTACLES_MAX) return nullptr;
		slot = &w->obstacles[w->obstacle_count++];
		break;
	}

	*slot = (entity){
		.kind = kind,
		.pos = at,
		.size = kind_size(kind),
		.life = kind_life(kind),
		.facing_right = true,
	};
	return slot;
}

// Swap-with-last. Caller must not advance its loop index on a removal.
static void remove_at(entity arena[static 1], int count[static 1], int index)
{
	arena[index] = arena[*count - 1];
	--*count;
}

static void reap(world w[static 1])
{
	for (int i = 0; i < w->projectile_count;) {
		if (w->projectiles[i].expiry <= 0.0f || w->projectiles[i].life <= 0)
			remove_at(w->projectiles, &w->projectile_count, i);
		else
			++i;
	}
	for (int i = 0; i < w->enemy_count;) {
		if (w->enemies[i].life <= 0)
			remove_at(w->enemies, &w->enemy_count, i);
		else
			++i;
	}
	for (int i = 0; i < w->player_count;) {
		if (w->players[i].life <= 0) {
			remove_at(w->players, &w->player_count, i);
			w->player_dead = true;
		} else {
			++i;
		}
	}
}

// Refill to the concurrency cap. ENEMY_CAP is a limit on enemies alive
// at once, not a level budget: the stage refills forever, exactly as the
// ancestor's Spawner did. ENEMIES_MAX is derived from ENEMY_CAP in
// world.h -- change one and you must change the other.
void world_respawn(world w[static 1], float dt)
{
	if (w->spawn_count == 0) return;

	w->respawn_timer -= dt;
	if (w->respawn_timer > 0.0f) return;
	w->respawn_timer = RESPAWN_INTERVAL;

	if (w->enemy_count >= ENEMY_CAP) return;

	// The stage has a budget as well as a cap. Spent, it sends nobody
	// else, and what is already alive is all that stands between the
	// player and the end.
	if (w->enemies_spawned >= STAGE_ENEMIES) return;

	vec2 at = w->spawn_points[w->spawn_cursor];
	w->spawn_cursor = (w->spawn_cursor + 1) % w->spawn_count;
	if (world_spawn(w, ENTITY_WARRIOR, at) != nullptr)
		++w->enemies_spawned;
}

// Fixed order, every frame: think, then resolve, then reap, then refill.
// Refill runs last because physics is where deaths happen -- refilling
// earlier lets an enemy killed this frame reappear in the same frame.
// An ended world stops. Nothing steps, nothing falls, nothing spawns: the
// last frame stays as it was, and a dialogue is drawn over it. Death and
// victory freeze identically -- the only difference is which dialogue the
// loop puts on top.
//
// Both flags are raised a step earlier than they are read here, so the
// frame the game ended in is complete before anything freezes.
void world_step(world w[static 1], float dt, input_state const in[static 1])
{
	if (w->player_dead || w->player_won)
		return;

	for (int i = 0; i < w->player_count; ++i)
		entity_step(&w->players[i], dt, in, w);
	for (int i = 0; i < w->enemy_count; ++i)
		entity_step(&w->enemies[i], dt, in, w);
	for (int i = 0; i < w->projectile_count; ++i)
		entity_step(&w->projectiles[i], dt, in, w);

	physics_apply(w, dt);
	reap(w);
	world_respawn(w, dt);

	// Won: the stage has sent its whole budget and the player has cleared
	// every one of them. Checked after the reap that emptied the arena and
	// after the spawner declined to refill it, so both are settled for this
	// frame. A player who dies clearing the last warrior stays dead: death
	// is decided in reap() above, and it wins the tie here rather than in
	// the loop, so no caller has to know the order to be correct.
	if (!w->player_dead && w->enemies_spawned >= STAGE_ENEMIES && w->enemy_count == 0)
		w->player_won = true;
}
