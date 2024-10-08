#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include <raylib.h>
#include <raymath.h>
#include <rcamera.h>

#include "chunk.h"
#include "world.h"
#include "entity.h"
#include "player.h"

#define DEFAULT_MOVEMENT_SPEED 100.0f
#define GROUND_FRICTION 15.0f
#define AIR_FRICTION 0.5f

player player_init(void) {

	player p = {
		.e = (entity){
			.position = {0,20,-5},
			.size = {.6,1.8,.6},
		},
		.movement_speed = DEFAULT_MOVEMENT_SPEED,
		.reach = 5,
		.camera_mode = FIRST_PERSON,
		.hp = 20,
		.hunger = 20,
		.is_flying = 0,
		.gamemode = MODE_SURVIVAL,
	};

	Camera3D* cam = malloc(sizeof(Camera3D));

	if (cam == NULL) {
		fprintf(stderr, "ERROR: Failed to allocate memory for player.cam\n");
		exit(1);
	}

	*cam = (Camera3D){
		.position = {0},
		.up = {0,1,0},
		.fovy = 70,
		.projection = CAMERA_PERSPECTIVE,
		.target = Vector3Add(p.e.position, (Vector3){.x = p.reach, 1.6f})
	};

	p.camera = cam;

	return p;
}

void player_destroy(player* player) {
	free(player->camera);
}

static void player_input(player* player) {
	if (IsKeyPressed(KEY_ESCAPE)) {
		switch (player->gamemode) {
			case (MODE_MENU):
			case (MODE_PAUSED):
				player->gamemode = MODE_SURVIVAL;
				break;
			default:
				player->gamemode = MODE_PAUSED;
		}
	}

	if (IsKeyPressed(KEY_E)) {
		switch (player->gamemode) {
			case (MODE_MENU):
			case (MODE_PAUSED):
				player->gamemode = MODE_SURVIVAL;
				break;
			default:
				player->gamemode = MODE_MENU;
		}
	}

	if (IsKeyPressed(KEY_ONE))
		player->gamemode = MODE_SURVIVAL;
	if (IsKeyPressed(KEY_TWO))
		player->gamemode = MODE_CREATIVE;
	if (IsKeyPressed(KEY_THREE)) {
		player->gamemode = MODE_SPECTATOR;
		player->is_flying = 1;
		player->e.is_on_ground = 0;
		player->e.velocity = Vector3Zero();
	}
			
	// Scroll through camera modes
	if (IsKeyPressed(KEY_F5)) {
		if (++player->camera_mode > THIRD_PERSON)
			player->camera_mode = 0;

		if (player->camera_mode == FIRST_PERSON) {
			player->camera->target = Vector3Add(player->camera->target, 
					Vector3Scale(
						Vector3Subtract(
							player->camera->position, 
							player->camera->target
							), 
						-2));
		}
	}
}

static void player_camera_movement(player* player) {
	// Camera movement
	const Vector2 mouse_delta = GetMouseDelta();
	const float camera_sensitivity = 0.007f;
	const float camera_y_offset = 1.6f;

	switch (player->camera_mode) {
		default:
		case (FIRST_PERSON):
			{
				CameraYaw(player->camera, camera_sensitivity * -mouse_delta.x, 0);
				CameraPitch(player->camera, camera_sensitivity * -mouse_delta.y, 1, 0, 0);

				Vector3 head_pos = Vector3Add(player->e.position, (Vector3){.y=camera_y_offset});
				player->camera->target = Vector3Add(head_pos, Vector3Scale(Vector3Normalize(GetCameraForward(player->camera)), player->reach));
				player->camera->position = head_pos;
			}
			break;
		case (THIRD_PERSON):
			{
				CameraYaw(player->camera, camera_sensitivity * -mouse_delta.x, 0);
				CameraPitch(player->camera, camera_sensitivity * -mouse_delta.y, 1, 0, 0);

				const float distance = 5; // distance of camera from player

				Vector3 head_pos = Vector3Add(player->e.position, (Vector3){.y=camera_y_offset});
				player->camera->position = Vector3Add(head_pos, Vector3Scale(Vector3Normalize(GetCameraForward(player->camera)), -distance));
				player->camera->target = head_pos;
			}
			break;
	}
}

static void player_movement(player* player) {
	float speed_multiplier = 1;

	if (player->gamemode == MODE_SPECTATOR)
		player->movement_speed = 100;
	else
		player->movement_speed = DEFAULT_MOVEMENT_SPEED;

	Camera3D unrotated_cam = *player->camera;
	unrotated_cam.target.y = unrotated_cam.position.y;

	Vector3 acceleration_delta = {0};

	if (IsKeyDown(KEY_W))
		acceleration_delta = Vector3Add(acceleration_delta, GetCameraForward(&unrotated_cam));
	if (IsKeyDown(KEY_S))
		acceleration_delta = Vector3Add(acceleration_delta, Vector3Scale(GetCameraForward(&unrotated_cam), -1));
	if (IsKeyDown(KEY_A))
		acceleration_delta = Vector3Add(acceleration_delta, Vector3Scale(GetCameraRight(&unrotated_cam), -1));
	if (IsKeyDown(KEY_D))
		acceleration_delta = Vector3Add(acceleration_delta, GetCameraRight(&unrotated_cam));

	if (player->is_flying) {
		if (player->gamemode == MODE_SURVIVAL || (IsKeyPressed(KEY_F) && player->gamemode == MODE_CREATIVE))
			player->is_flying = 0;

		if (IsKeyDown(KEY_SPACE))
			acceleration_delta = Vector3Add(acceleration_delta, GetCameraUp(&unrotated_cam));
		if (IsKeyDown(KEY_LEFT_SHIFT))
			acceleration_delta = Vector3Add(acceleration_delta, Vector3Scale(GetCameraUp(&unrotated_cam), -1));
	} else {
		if (IsKeyPressed(KEY_F) && player->gamemode == MODE_CREATIVE) {
			player->is_flying = 1;
			player->e.velocity = Vector3Zero();
		}
	}

	// Jumping
	if (player->e.is_on_ground) {
		player->is_flying = 0;
		if (IsKeyDown(KEY_SPACE)) {
			player->e.is_on_ground = 0;
			entity_add_force(&player->e, (Vector3){.y=12});
		}
	} else if (!player->is_flying)
		speed_multiplier *= 0.1;

	// Sprint
	if (IsKeyDown(KEY_LEFT_CONTROL))
		speed_multiplier *= 1.4;
	
	acceleration_delta = Vector3Normalize(acceleration_delta);
	acceleration_delta = Vector3Scale(acceleration_delta, player->movement_speed * speed_multiplier);

	// apply movement
	entity_add_force(&player->e, acceleration_delta);

}

static void player_physics(player* player) {
	const float delta_t = GetFrameTime();

	// Friction
	if (!Vector3Equals(player->e.velocity, Vector3Zero())) {
		if (player->e.is_on_ground)
			entity_add_force(&player->e, Vector3Scale(player->e.velocity, -GROUND_FRICTION));
		else
			entity_add_force(&player->e, Vector3Scale(player->e.velocity, -AIR_FRICTION));
	}

	if (FloatEquals(player->e.velocity.x, 0))
		player->e.velocity.x = 0;
	if (FloatEquals(player->e.velocity.y, 0))
		player->e.velocity.y = 0;
	if (FloatEquals(player->e.velocity.z, 0))
		player->e.velocity.z = 0;

	if (player->gamemode != MODE_SPECTATOR) {
		// Gravity
		if (!player->is_flying) {
			// const float g = -9.81;
			const float g = -45;
			entity_add_force(&player->e, (Vector3){.y = g});
		} 

		// Collision
		entity_block_collision(&player->e);
	}

	// apply velocity to position, MUST BE LAST
	player->e.position = Vector3Add(player->e.position, Vector3Scale(player->e.velocity, delta_t));
}

// update method for player
void player_update(player* player) {
	static bool is_cursor_enabled = 0;

	// generic inputs (change gamemode camera mode etc.)
	player_input(player);

	if (player->gamemode == MODE_MENU || player->gamemode == MODE_PAUSED) {
		if (!is_cursor_enabled) {
			EnableCursor();
			is_cursor_enabled = 1;
		}

		if (player->gamemode == MODE_MENU) {
			// TODO prevent player movement in menu mode
			player_physics(player);
		}

	} else {
		if (is_cursor_enabled) {
			DisableCursor();
			is_cursor_enabled = 0;
		}

		// handle player input movement
		player_movement(player);
		// apply physics (gravity, velocity etc.)
		player_physics(player);
		player_camera_movement(player);

	}
}
