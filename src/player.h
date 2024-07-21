#pragma once

#include <raylib.h>

#include "items.h"
#include "world.h"


typedef enum {
	MODE_SURVIVAL,
	MODE_CREATIVE,
	MODE_SPECTATOR,
	MODE_PAUSED,
	MODE_MENU,
	MODE_TITLESCREEN,
} gamemode_type;

typedef struct {
	entity e;
	Vector3 input_vector; // velocity from player inputs
	gamemode_type gamemode;
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

player player_init(void);
void player_destroy(player* p);

// called every frame
void player_update(player* player);
