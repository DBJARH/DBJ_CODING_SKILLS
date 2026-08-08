/*
    2026AUG08       (c) dbj@dbj.org

    Permanent tau suite for dbj_the_game. See docs/implementation_milestone_one.md.

    This binary links world.c, entity.c, physics.c and map.c -- and no
    raylib. That is not an optimisation, it is the assertion: if the
    simulation ever reaches for a window, this file stops linking. Every
    test below drives world_step() directly with a scripted input_state,
    exactly as a recorded input array would.

    Worlds are built in code rather than loaded from assets/, so no test
    depends on a working directory.
*/
#include <math.h>

#include "../entity.h"
#include "../physics.h"
#include "../world.h"

#include <tau/tau.h>
TAU_MAIN()

#define STEP_DT (1.0f / 60.0f)

static void run(world wld[static 1], input_state const in[static 1], int frames)
{
	for (int frame = 0; frame < frames; ++frame)
		world_step(wld, STEP_DT, in);
}

// A mover falls until a platform stops it, and stays stopped.
TEST(physics, gravity_settles_on_platform) {
	world wld = {0};
	world_spawn(&wld, ENTITY_PLATFORM, (vec2){0.0f, 200.0f});
	entity *player = world_spawn(&wld, ENTITY_PLAYER, (vec2){10.0f, 100.0f});
	REQUIRE_TRUE(player != nullptr, "player slot must exist in an empty world");

	input_state idle = {0};
	run(&wld, &idle, 120);

	REQUIRE_TRUE(player->grounded, "a mover resting on a platform is grounded");
	REQUIRE_TRUE(fabsf((player->pos.y + player->size.y) - 200.0f) < 1.0f,
	             "the mover's feet must come to rest on the platform top");
}

// Walking into a wall stops the walker at the wall, not inside it.
TEST(physics, wall_stops_a_walker) {
	world wld = {0};
	world_spawn(&wld, ENTITY_PLATFORM, (vec2){0.0f, 200.0f});
	world_spawn(&wld, ENTITY_PLATFORM, (vec2){40.0f, 200.0f});
	world_spawn(&wld, ENTITY_PLATFORM, (vec2){80.0f, 160.0f});
	entity *player = world_spawn(&wld, ENTITY_PLAYER, (vec2){10.0f, 157.0f});
	REQUIRE_TRUE(player != nullptr, "player slot must exist in an empty world");

	input_state walk_right = {.right = true};
	run(&wld, &walk_right, 180);

	REQUIRE_TRUE(player->pos.x + player->size.x <= 80.5f,
	             "a walker must stop at the wall face, not tunnel through it");
}

// A knife on top of a dying warrior kills it, and the reap pass collects
// both in the same frame.
TEST(world, knife_kills_and_both_are_reaped) {
	world wld = {0};
	entity *foe = world_spawn(&wld, ENTITY_WARRIOR, (vec2){100.0f, 100.0f});
	entity *knife = world_spawn(&wld, ENTITY_PROJECTILE, (vec2){100.0f, 100.0f});
	REQUIRE_TRUE(foe != nullptr && knife != nullptr, "both slots must exist");
	foe->life = 1;
	knife->expiry = 1.0f;

	input_state idle = {0};
	run(&wld, &idle, 1);

	REQUIRE_TRUE(wld.enemy_count == 0, "a warrior out of life is reaped");
	REQUIRE_TRUE(wld.projectile_count == 0, "a spent knife is reaped with it");
}

// ENEMY_CAP bounds enemies alive at once. The stage refills forever, so
// the cap is the only thing between the spawner and a full arena.
TEST(world, spawner_honours_the_cap) {
	world wld = {0};
	wld.spawn_points[0] = (vec2){0.0f, 0.0f};
	wld.spawn_count = 1;

	input_state idle = {0};
	run(&wld, &idle, 60 * 120);

	REQUIRE_TRUE(wld.enemy_count == ENEMY_CAP,
	             "a stage left running fills to the cap");
	REQUIRE_TRUE(wld.enemy_count <= ENEMIES_MAX,
	             "and never past the arena it is derived from");
}

// A full arena refuses out loud. Silent dropping is the bug the fixed
// arena would otherwise trade a leak for.
TEST(world, full_arena_refuses) {
	world wld = {0};
	for (int i = 0; i < PROJECTILES_MAX; ++i) {
		REQUIRE_TRUE(world_spawn(&wld, ENTITY_PROJECTILE, (vec2){0.0f, 0.0f}) != nullptr,
		             "every slot up to the bound must be handed out");
	}
	REQUIRE_TRUE(world_spawn(&wld, ENTITY_PROJECTILE, (vec2){0.0f, 0.0f}) == nullptr,
	             "the slot past the bound must be refused, not overwritten");
}

// Timers belong to the entity, not to the presence of a player. With the
// player dead there is nothing to chase and nothing to think about, but a
// clock that has been started still has to run down.
TEST(entity, warrior_flash_decays_with_no_player) {
	world wld = {0};
	entity *foe = world_spawn(&wld, ENTITY_WARRIOR, (vec2){0.0f, 0.0f});
	REQUIRE_TRUE(foe != nullptr, "warrior slot must exist in an empty world");
	foe->hurt_flash = 0.5f;

	input_state idle = {0};
	run(&wld, &idle, 1);

	REQUIRE_TRUE(foe->hurt_flash < 0.5f,
	             "hurt_flash must decay even when no player is alive");
}
