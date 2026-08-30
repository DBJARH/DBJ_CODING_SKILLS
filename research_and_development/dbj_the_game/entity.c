#include "entity.h"

#define PLAYER_ACCEL      900.0f
#define PLAYER_MAX_SPEED  180.0f
#define PLAYER_JUMP_SPEED 420.0f
#define PLAYER_FRICTION   0.82f
#define WARRIOR_SPEED      60.0f
#define KNIFE_SPEED       320.0f
#define KNIFE_COOLDOWN      0.5f
#define KNIFE_LIFETIME      1.5f
// Clearing a step of one CELL means the FEET -- pos.y + size.y, not
// pos.y, which is the head -- must rise above the wall top. Apex is
// v^2/2g, and the step integrator loses about g*dt/2 of it, so
// v > sqrt(2 * GRAVITY * CELL) + GRAVITY * dt / 2, which is ~320 at
// 60 fps. 340 for margin: it peaks the feet at 154.67 against a wall
// top of 160. Two stacked cells would need 460, above the player's own
// PLAYER_JUMP_SPEED -- castle.txt has no stacked platforms, so no wall
// in the shipping stage asks for it.
#define WARRIOR_HOP_SPEED  340.0f

static entity *nearest_player(world w[static 1], vec2 from)
{
	entity *best = nullptr;
	float best_dist = 0.0f;
	for (int i = 0; i < w->player_count; ++i) {
		entity *cand = &w->players[i];
		float dx = cand->pos.x - from.x;
		float dist = dx < 0 ? -dx : dx;
		if (!best || dist < best_dist) {
			best = cand;
			best_dist = dist;
		}
	}
	return best;
}

static void step_player(entity ent[static 1], float dt,
                        input_state const in[static 1], world w[static 1])
{
	if (in->left) {
		ent->vel.x -= PLAYER_ACCEL * dt;
		ent->facing_right = false;
	}
	if (in->right) {
		ent->vel.x += PLAYER_ACCEL * dt;
		ent->facing_right = true;
	}
	if (!in->left && !in->right)
		ent->vel.x *= PLAYER_FRICTION;

	if (ent->vel.x > PLAYER_MAX_SPEED) ent->vel.x = PLAYER_MAX_SPEED;
	if (ent->vel.x < -PLAYER_MAX_SPEED) ent->vel.x = -PLAYER_MAX_SPEED;

	if (in->jump && ent->grounded) {
		ent->vel.y = -PLAYER_JUMP_SPEED;
		ent->grounded = false;
	}

	if (ent->fire_cooldown > 0.0f)
		ent->fire_cooldown -= dt;
	if (in->attack && ent->fire_cooldown <= 0.0f) {
		vec2 muzzle = {
			ent->pos.x + (ent->facing_right ? ent->size.x : -8.0f),
			ent->pos.y + ent->size.y * 0.4f,
		};
		entity *knife = world_spawn(w, ENTITY_PROJECTILE, muzzle);
		if (knife) {
			knife->vel.x = ent->facing_right ? KNIFE_SPEED : -KNIFE_SPEED;
			knife->expiry = KNIFE_LIFETIME;
		}
		ent->fire_cooldown = KNIFE_COOLDOWN;
	}

	if (ent->hurt_flash > 0.0f)
		ent->hurt_flash -= dt;

	// Beside hurt_flash for the same reason: a clock decays with time,
	// not with whether the thing that started it is still there. Ticked
	// only inside the fire it would freeze on the way out, and a player
	// who left at 0.4 remaining would still owe that 0.4 on returning
	// an hour later.
	if (ent->burn_cooldown > 0.0f)
		ent->burn_cooldown -= dt;

	player_motion was = ent->motion;
	if (!ent->grounded)
		ent->motion = PLAYER_JUMP;
	else if (in->left || in->right)
		ent->motion = PLAYER_WALK;
	else
		ent->motion = PLAYER_REST;

	// The clock belongs to the motion, so entering a motion restarts it --
	// otherwise a walk resumed after a jump lands mid-stride.
	ent->anim_time = (ent->motion == was) ? ent->anim_time + dt : 0.0f;
}

static void step_warrior(entity ent[static 1], float dt, world w[static 1])
{
	// Clocks run before decisions: a timer already started belongs to the
	// entity, not to whether there is anything left to chase.
	if (ent->hurt_flash > 0.0f)
		ent->hurt_flash -= dt;

	// Read before this frame's decision overwrites it: a warrior that
	// was told to walk and has vel.x == 0 anyway was stopped by
	// something, and the only thing that stops X is the collision pass.
	bool was_chasing = ent->motion == PLAYER_WALK;

	entity *target = nearest_player(w, ent->pos);
	if (!target) {
		ent->vel.x = 0.0f;
		ent->motion = PLAYER_REST;
		return;
	}
	ent->motion = PLAYER_WALK;
	// Blocked-and-grounded means the wall or ledge in front won last
	// frame: the mover wanted to move on X and collision zeroed it. The
	// warrior hops. Deciding this here, not in the collision pass, is
	// the point -- physics reports what happened, the AI decides what
	// to do about it.
	bool blocked = ent->grounded && ent->vel.x == 0.0f && was_chasing;

	bool right = target->pos.x > ent->pos.x;
	ent->facing_right = right;
	ent->vel.x = right ? WARRIOR_SPEED : -WARRIOR_SPEED;

	if (blocked)
		ent->vel.y = -WARRIOR_HOP_SPEED;
}

static void step_projectile(entity ent[static 1], float dt)
{
	ent->expiry -= dt;
}

void entity_step(entity ent[static 1], float dt, input_state const in[static 1],
                 world w[static 1])
{
	switch (ent->kind) {
	case ENTITY_PLAYER:
		step_player(ent, dt, in, w);
		break;
	case ENTITY_WARRIOR:
		step_warrior(ent, dt, w);
		break;
	case ENTITY_PROJECTILE:
		step_projectile(ent, dt);
		break;
	case ENTITY_PLATFORM:
		break;
	case ENTITY_SPIKE:
		break;
	case ENTITY_FIRE:
		break;
	}
}

bool entity_overlaps(entity const a[static 1], entity const b[static 1])
{
	return a->pos.x < b->pos.x + b->size.x &&
	       a->pos.x + a->size.x > b->pos.x &&
	       a->pos.y < b->pos.y + b->size.y &&
	       a->pos.y + a->size.y > b->pos.y;
}
