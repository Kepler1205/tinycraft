#include <stdlib.h>

#include <raylib.h>
#include <raymath.h>
#include <rcamera.h>

#include "player.h"

#define DEFAULT_MOVEMENT_SPEED 5

extern player player_init(void) {
	Camera3D* cam = malloc(sizeof(Camera3D));

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
		.camera = cam,
	};

	*cam = (Camera3D){
		.position = {0},
		.up = {0,1,0},
		.fovy = 70,
		.projection = CAMERA_PERSPECTIVE,
		.target = Vector3Add(p.e.position, (Vector3){.x = p.reach, 1.6f})
	};

	return p;
}

extern void player_destroy(player* player) {
	free(player->camera);
}

extern void player_movement(player* player) {
	// DEBUG
	if (IsKeyPressed(KEY_R)) {
		player_set_position(player, Vector3Zero());
		player->camera->target = (Vector3){player->reach, 1.6f, 0};
	}

	if (IsKeyPressed(KEY_ONE))
		player->gamemode = MODE_SURVIVAL;
	if (IsKeyPressed(KEY_TWO))
		player->gamemode = MODE_CREATIVE;
	if (IsKeyPressed(KEY_THREE)) {
		player->gamemode = MODE_SPECTATOR;
		player->is_flying = 1;
		player->is_on_ground = 0;
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

	// Player movement
	Vector3 velocity_delta = {0};

	float speed_multiplier = 1;

	if (player->gamemode == MODE_SPECTATOR)
		player->movement_speed = 20;
	else
		player->movement_speed = DEFAULT_MOVEMENT_SPEED;

	Camera3D unrotated_cam = *player->camera;
	unrotated_cam.target.y = unrotated_cam.position.y;

	if (IsKeyDown(KEY_W))
		velocity_delta = Vector3Add(velocity_delta, GetCameraForward(&unrotated_cam));
	if (IsKeyDown(KEY_S))
		velocity_delta = Vector3Add(velocity_delta, Vector3Scale(GetCameraForward(&unrotated_cam), -1));
	if (IsKeyDown(KEY_A))
		velocity_delta = Vector3Add(velocity_delta, Vector3Scale(GetCameraRight(&unrotated_cam), -1));
	if (IsKeyDown(KEY_D))
		velocity_delta = Vector3Add(velocity_delta, GetCameraRight(&unrotated_cam));

	if (player->is_flying) {
		if (player->gamemode == MODE_SURVIVAL || (IsKeyPressed(KEY_F) && player->gamemode == MODE_CREATIVE))
			player->is_flying = 0;
		if (IsKeyDown(KEY_SPACE))
			velocity_delta = Vector3Add(velocity_delta, GetCameraUp(&unrotated_cam));
		if (IsKeyDown(KEY_LEFT_SHIFT))
			velocity_delta = Vector3Add(velocity_delta, Vector3Scale(GetCameraUp(&unrotated_cam), -1));
	} else {
		if (IsKeyPressed(KEY_F) && player->gamemode == MODE_CREATIVE) {
			player->is_flying = 1;
			player->e.velocity = Vector3Zero();
		}
	}

	// Jumping
	if (player->is_on_ground) {
		player->is_flying = 0;
		if (IsKeyDown(KEY_SPACE)) {
			player->is_on_ground = 0;
			player_add_impulse(player, (Vector3){.y=6});
		}
	} else if (!player->is_flying)
		speed_multiplier *= 0.7;

	// Sprint
	if (IsKeyDown(KEY_LEFT_CONTROL))
		speed_multiplier *= 1.4;

	// Stops multiple inputs from increasing speed
	velocity_delta = Vector3Normalize(velocity_delta);
	velocity_delta = Vector3Scale(velocity_delta, player->movement_speed * speed_multiplier);

	player->input_vector = velocity_delta;
	// player_add_position(player, Vector3Scale(player->e.velocity, delta_t));
	

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

				player->camera->position = Vector3Add(player->e.position, (Vector3){.y=camera_y_offset}); // head position offset
				player->camera->target = Vector3Add(player->camera->position, Vector3Scale(Vector3Normalize(GetCameraForward(player->camera)), player->reach));
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

extern void player_physics(player* player) {
	const float delta_t = GetFrameTime();

	// Gravity
	if (!(player->is_on_ground || player->is_flying || player->gamemode == MODE_SPECTATOR)) {
		// const float g = -9.81;
		const float g = -14;
		const float terminal_velocity = -2000;
		if (player->e.velocity.y > terminal_velocity)
			player_add_force(player, (Vector3){.y = g});
	} 

	// apply velocity to position
	player_add_position(player, 
			Vector3Scale(
				Vector3Add(
					player->e.velocity,
					player->input_vector
					),
			delta_t));
}

extern void player_set_position(player* player, Vector3 position) {
	const Vector3 diff = Vector3Subtract(player->e.position, position);
	player->e.position = Vector3Subtract(player->e.position, diff);
	player->camera->position = Vector3Subtract(player->camera->position, diff);
	player->camera->target = Vector3Subtract(player->camera->target, diff);
}

extern void player_add_position(player* player, Vector3 position_delta) {
	player_set_position(player, Vector3Add(player->e.position, position_delta));
}

// called per-frame to add a force vector
extern void player_add_force(player* player, Vector3 force) {
	const float delta_t = GetFrameTime();
	if (!Vector3Equals(force, Vector3Zero()))
		player->e.velocity = Vector3Add(player->e.velocity, Vector3Scale(force, delta_t));
}

// called once to add an impulse (instantaeious force)
extern void player_add_impulse(player* player, Vector3 force) {
	if (!Vector3Equals(force, Vector3Zero()))
		player->e.velocity = Vector3Add(player->e.velocity, force);
}

void player_collide(player* player, Vector3 block_pos) {
	aabb_collision_result res = entity_block_collision(&player->e, block_pos);
	if (res.collided) {
		if (res.collision_depth.y != 0.0f) {
			player->is_on_ground = 1;
			// fixes bouncing on the ground
			// player_add_position(&player, (Vector3){.y=res.collision_depth.y-0.001});
		} else
			player_add_position(player, res.collision_depth);
	} 

}
