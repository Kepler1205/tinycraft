#pragma once

#include <raylib.h>

#include "items.h"
#include "world.h"

typedef struct {
	entity e;
	Vector3 position; // do not edit directly, use player_set_position()
	Vector3 velocity; // velocity from external forces (gravity, knockback)
	Vector3 input_vector; // velocity from player inputs
	unsigned int hp;
	unsigned int hunger;
	float movement_speed;
	float reach;
	bool is_flying;
	bool is_on_ground;
	enum {
		FIRST_PERSON = 0,
		THIRD_PERSON,
	} camera_mode;
	Camera* camera;
	item hotbar[9];
	item inventory[9][4];
} player;

extern player player_init(void);
extern void player_destroy(player* p);

// called every frame
extern void player_movement(player* player);
extern void player_physics(player* player);
extern void player_add_force(player* player, Vector3 force);

// called once
extern void player_add_impulse(player* player, Vector3 force);
extern void player_add_position(player* player, Vector3 position_delta);
extern void player_set_position(player* player, Vector3 position);
