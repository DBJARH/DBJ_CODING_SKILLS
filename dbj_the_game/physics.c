#include "physics.h"
#include "entity.h"

#define GRAVITY       1200.0f
#define TERMINAL_VEL   600.0f
#define TOUCH_DAMAGE     2
#define KNIFE_DAMAGE     5
#define SPIKE_DAMAGE     3
#define FIRE_DAMAGE      1  // per hurt() window, not per frame

// Resolve one mover against one solid box on whichever axis it entered
// least far. Cheap, and adequate for axis-aligned tiles.
// Axis-separated resolution: move on one axis, resolve that axis, then
// the other. Resolving a 2D overlap in one pass has to guess which axis
// the mover entered by, and it guesses wrong at every tile seam -- a
// mover walking a flat floor gets shunted sideways and stops dead in
// open ground. Doing one axis at a time removes the guess entirely.
static void resolve_axis_x(entity mover[static 1], entity const box[static 1])
{
	float overlap_left  = (mover->pos.x + mover->size.x) - box->pos.x;
	float overlap_right = (box->pos.x + box->size.x) - mover->pos.x;

	if (overlap_left < overlap_right)
		mover->pos.x -= overlap_left;
	else
		mover->pos.x += overlap_right;
	mover->vel.x = 0.0f;
}

static void resolve_axis_y(entity mover[static 1], entity const box[static 1])
{
	float overlap_top    = (mover->pos.y + mover->size.y) - box->pos.y;
	float overlap_bottom = (box->pos.y + box->size.y) - mover->pos.y;

	if (overlap_top < overlap_bottom) {
		mover->pos.y -= overlap_top;
		mover->grounded = true;        // landed on top of the box
	} else {
		mover->pos.y += overlap_bottom;
	}
	mover->vel.y = 0.0f;
}

// One mover against every platform, X then Y. Hazards are reported
// through `hazard` rather than resolved -- they do not block. The two
// hazard kinds differ in what they cost, so they are reported apart:
// a spike is a thing you touch, a fire is a thing you stand in.
static void collide_map(entity mover[static 1], float dt, world w[static 1],
                        bool touched_spike[static 1], bool in_fire[static 1])
{
	mover->pos.x += mover->vel.x * dt;
	for (int j = 0; j < w->obstacle_count; ++j) {
		entity *box = &w->obstacles[j];
		if (box->kind != ENTITY_PLATFORM) continue;
		if (entity_overlaps(mover, box)) resolve_axis_x(mover, box);
	}

	mover->pos.y += mover->vel.y * dt;
	for (int j = 0; j < w->obstacle_count; ++j) {
		entity *box = &w->obstacles[j];
		if (box->kind != ENTITY_PLATFORM) continue;
		if (entity_overlaps(mover, box)) resolve_axis_y(mover, box);
	}

	for (int j = 0; j < w->obstacle_count; ++j) {
		entity const *box = &w->obstacles[j];
		switch (box->kind) {
		case ENTITY_SPIKE:
			if (entity_overlaps(mover, box)) *touched_spike = true;
			break;
		case ENTITY_FIRE:
			if (entity_overlaps(mover, box)) *in_fire = true;
			break;
		case ENTITY_PLATFORM:
		case ENTITY_PLAYER:
		case ENTITY_WARRIOR:
		case ENTITY_PROJECTILE:
			break;
		}
	}
}

static void apply_gravity(entity ent[static 1], float dt)
{
	ent->vel.y += GRAVITY * dt;
	if (ent->vel.y > TERMINAL_VEL)
		ent->vel.y = TERMINAL_VEL;
}

static void integrate(entity ent[static 1], float dt)
{
	ent->pos.x += ent->vel.x * dt;
	ent->pos.y += ent->vel.y * dt;
}

static void hurt(entity victim[static 1], int amount)
{
	if (victim->hurt_flash > 0.0f) return;   // invulnerable window
	victim->life -= amount;
	victim->hurt_flash = HURT_FLASH_TIME;
}

void physics_apply(world w[static 1], float dt)
{
	// Movers fall, then move and resolve one axis at a time. Obstacles
	// never move, so they are not touched here at all.
	for (int i = 0; i < w->player_count; ++i) {
		entity *player = &w->players[i];
		player->grounded = false;
		apply_gravity(player, dt);

		// A spike bites once when touched; a fire burns for as long as
		// the player stands in it. Both go through hurt(), so the
		// invulnerability window is what paces the burn -- FIRE_DAMAGE
		// lands once per window, not once per frame.
		bool touched_spike = false;
		bool in_fire = false;
		collide_map(player, dt, w, &touched_spike, &in_fire);
		if (touched_spike) hurt(player, SPIKE_DAMAGE);
		else if (in_fire)  hurt(player, FIRE_DAMAGE);
	}

	for (int i = 0; i < w->enemy_count; ++i) {
		entity *foe = &w->enemies[i];
		foe->grounded = false;
		apply_gravity(foe, dt);

		bool touched_spike = false;   // hazards do not harm enemies
		bool in_fire = false;
		collide_map(foe, dt, w, &touched_spike, &in_fire);
	}

	for (int i = 0; i < w->projectile_count; ++i)
		integrate(&w->projectiles[i], dt);

	// Player vs enemy: touch damage.
	for (int i = 0; i < w->player_count; ++i) {
		for (int j = 0; j < w->enemy_count; ++j) {
			if (entity_overlaps(&w->players[i], &w->enemies[j]))
				hurt(&w->players[i], TOUCH_DAMAGE);
		}
	}

	// Knives vs enemies. Both die; the reap pass collects them.
	//
	// These two lines deliberately do not call hurt(): a knife ignores
	// the invulnerability window, so two knives landing in one frame
	// both count. That is what keeps ranged worth throwing. It is a
	// decision, not an oversight -- do not "fix" it into hurt().
	for (int i = 0; i < w->projectile_count; ++i) {
		entity *knife = &w->projectiles[i];
		for (int j = 0; j < w->enemy_count; ++j) {
			if (!entity_overlaps(knife, &w->enemies[j])) continue;
			w->enemies[j].life -= KNIFE_DAMAGE;
			w->enemies[j].hurt_flash = HURT_FLASH_TIME;
			knife->life = 0;
			break;
		}
	}

	// Knives stop at walls.
	for (int i = 0; i < w->projectile_count; ++i) {
		for (int j = 0; j < w->obstacle_count; ++j) {
			if (w->obstacles[j].kind != ENTITY_PLATFORM) continue;
			if (entity_overlaps(&w->projectiles[i], &w->obstacles[j])) {
				w->projectiles[i].life = 0;
				break;
			}
		}
	}
}
