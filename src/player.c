#include <stdlib.h>

#include <raylib.h>
#include <raymath.h>
#include <rcamera.h>

#include "player.h"

#define DEFAULT_MOVEMENT_SPEED 5

player player_init(void) {
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
		.gamemode = MODE_SURVIVAL,
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

void player_destroy(player* player) {
	free(player->camera);
}

static inline void player_set_position(player* player, Vector3 position) {
	const Vector3 diff = Vector3Subtract(player->e.position, position);
	player->e.position = Vector3Subtract(player->e.position, diff);
	player->camera->position = Vector3Subtract(player->camera->position, diff);
	player->camera->target = Vector3Subtract(player->camera->target, diff);
}

static inline void player_add_position(player* player, Vector3 position_delta) {
	player_set_position(player, Vector3Add(player->e.position, position_delta));
}

// called per-frame to add a force vector
static inline void player_add_force(player* player, Vector3 force) {
	const float delta_t = GetFrameTime();
	if (!Vector3Equals(force, Vector3Zero()))
		player->e.velocity = Vector3Add(player->e.velocity, Vector3Scale(force, delta_t));
}

// called once to add an impulse (instantaeious force)
static inline void player_add_impulse(player* player, Vector3 force) {
	if (!Vector3Equals(force, Vector3Zero()))
		player->e.velocity = Vector3Add(player->e.velocity, force);
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
}

static void player_movement(player* player) {
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

static void player_block_collision(player* player) {
	// chunk that the player is inside
	world_chunk_pos player_chunk_pos = { 
		floorf(player->e.position.x / 16.0f),
		floorf(player->e.position.z / 16.0f),
	};

	// check surrounding chunks for distance
	for (int x = -1; x <= 1; x++) {
	for (int z = -1; z <= 1; z++) {
		world_chunk_pos pos = (world_chunk_pos){x + player_chunk_pos.x, z + player_chunk_pos.z};
		chunk* chunk = world_chunk_lookup(pos);

		// don't check unloaded chunks
		if (chunk == NULL)
			continue;

		// positions of blocks in chunk
		int cx = floorf(player->e.position.x) - pos.x * WORLD_CHUNK_WIDTH;
		int cy = ceilf(player->e.position.y);
		int cz = floorf(player->e.position.z) - pos.z * WORLD_CHUNK_WIDTH;

		// only check chunks that are close
		if (cx >= WORLD_CHUNK_WIDTH + 2 || cz >= WORLD_CHUNK_WIDTH + 2)
			continue;

		for (int ix = -1; ix <= 1; ix++) {
		for (int iy =  0; iy <= 2; iy++) {
		for (int iz = -1; iz <= 1; iz++) {
			int bx = cx + ix;
			int by = cy + iy;
			int bz = cz + iz;

			// printf("bx: %d by: %d bz: %d\n", bx,by,bz);

			if (
					bx < 0 || bx > WORLD_CHUNK_WIDTH  - 1 ||
					by < 0 || by > WORLD_CHUNK_HEIGHT - 1 ||
					bz < 0 || bz > WORLD_CHUNK_WIDTH  - 1
			   )
				continue;

			// DEBUG Show collision check boundry
			/* BeginMode3D(*player->camera);
			Vector3 b_pos = get_block_real_pos(pos, bx, by, bz);
			b_pos.x -= .5;
			b_pos.y -= .5;
			b_pos.z -= .5;
			DrawCubeV(b_pos, Vector3One(), (Color){.a = 50, .b = 240});
			Vector3 c_pos = get_block_real_pos(pos, cx, cy, cz);
			c_pos.x -= .5;
			c_pos.y -= .5;
			c_pos.z -= .5;
			DrawCubeV(c_pos, Vector3One(), (Color){.a = 50, .r = 240});
			EndMode3D(); */

			Vector3 block_pos = get_block_real_pos(pos, bx, by, bz);
			aabb_collision_result res = entity_block_collision(&player->e, block_pos);

			if (chunk->blocks[bx][by][bz].id == 0)
				continue;

			if (res.collided) {
				if (res.collision_depth.y != 0.0f) {
					player->is_on_ground = 1;
					// fixes bouncing on the ground
					// player_add_position(&player, (Vector3){.y=res.collision_depth.y-0.001});
				} else
					player_add_position(player, res.collision_depth);
			}
		}}}
	}}
}

static void player_physics(player* player) {
	const float delta_t = GetFrameTime();

	// apply velocity to position
	player_add_position(player, 
			Vector3Scale(
				Vector3Add(
					player->e.velocity,
					player->input_vector
					),
			delta_t));

	if (player->gamemode == MODE_SPECTATOR)
		return;

	// Gravity
	if (!(player->is_on_ground || player->is_flying)) {
		// const float g = -9.81;
		const float g = -14;
		const float terminal_velocity = -2000;
		if (player->e.velocity.y > terminal_velocity)
			player_add_force(player, (Vector3){.y = g});
	} 
	// Collision
	player->is_on_ground = 0;
	player_block_collision(player);

	if (player->is_on_ground)
		player->e.velocity.y = 0;
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
			player->input_vector = Vector3Zero();
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
	}
}
