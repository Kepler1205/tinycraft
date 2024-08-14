#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include <raylib.h>
#include <raymath.h>
#include <rcamera.h>

#include "player.h"
#include "chunk.h"
#include "world.h"

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
	if (player->is_on_ground) {
		player->is_flying = 0;
		if (IsKeyDown(KEY_SPACE)) {
			player->is_on_ground = 0;
			player_add_impulse(player, (Vector3){.y=12});
		}
	} else if (!player->is_flying)
		speed_multiplier *= 0.1;

	// Sprint
	if (IsKeyDown(KEY_LEFT_CONTROL))
		speed_multiplier *= 1.4;
	
	acceleration_delta = Vector3Normalize(acceleration_delta);
	acceleration_delta = Vector3Scale(acceleration_delta, player->movement_speed * speed_multiplier);

	// apply movement
	player_add_force(player, acceleration_delta);

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

#include <stdio.h>
static void player_block_collision(player* player) {

	// collision has to be checked 3 times
	// once for each axis
	for (unsigned int axis = 0; axis < 3; axis++) {

		// chunk that the player is inside
		world_chunk_pos player_chunk_pos = { 
			floorf(player->e.position.x / WORLD_CHUNK_WIDTH),
			floorf(player->e.position.z / WORLD_CHUNK_WIDTH),
		};

		chunk* player_chunk = world_chunk_lookup(player_chunk_pos);
		
		if (player_chunk == NULL) {
			fprintf(stderr, "WARNING: Player is inside an unloded chunk\n");
			return;
		}

		float delta_t = GetFrameTime();

		RayCollision nearest_collision = {
			.distance = INFINITY,
			.hit = 0,
		};
		// ------- broadphase calculation -------
		Vector3 frame_travel = Vector3Scale(player->e.velocity, delta_t);

		// broadphase box
		BoundingBox bphase = {
			.max = Vector3Add(player->e.position, frame_travel),
			.min = player->e.position,
		};

		const float padding = 0.0f;

		// adjustments to include entire entity in
		// current and next frame
		if (player->e.velocity.x > 0) {
			bphase.min.x -= (player->e.size.x + padding) * 0.5f;
			bphase.max.x += (player->e.size.x + padding) * 0.5f;
		} else {
			bphase.min.x += (player->e.size.x + padding) * 0.5f;
			bphase.max.x -= (player->e.size.x + padding) * 0.5f;
		}

		if (player->e.velocity.y > 0) {
			bphase.max.y += (player->e.size.y + padding);
		} else {
			bphase.min.y += (player->e.size.y + padding);
		}

		if (player->e.velocity.z > 0) {
			bphase.min.z -= (player->e.size.z + padding) * 0.5f;
			bphase.max.z += (player->e.size.z + padding) * 0.5f;
		} else {
			bphase.min.z += (player->e.size.z + padding) * 0.5f;
			bphase.max.z -= (player->e.size.z + padding) * 0.5f;
		}

		int start_x, start_y, start_z;
		int end_x, end_y, end_z;

		if (bphase.min.x > bphase.max.x) {
			start_x = floorf(bphase.max.x);
			end_x = ceilf(bphase.min.x);
		} else {
			start_x = floorf(bphase.min.x);
			end_x = ceilf(bphase.max.x); 
		}

		if (bphase.min.y > bphase.max.y) {
			start_y = floorf(bphase.max.y);
			end_y = ceilf(bphase.min.y);
		} else {
			start_y = floorf(bphase.min.y);
			end_y = ceilf(bphase.max.y); 
		}

		if (bphase.min.z > bphase.max.z) {
			start_z = floorf(bphase.max.z);
			end_z = ceilf(bphase.min.z);
		} else {
			start_z = floorf(bphase.min.z);
			end_z = ceilf(bphase.max.z); 
		}

		printf("bphase.max %f %f %f\n", bphase.max.x, bphase.max.y, bphase.max.z);
		printf("bphase.min %f %f %f\n", bphase.min.x, bphase.min.y, bphase.min.z);
		
		printf("start %d %d %d\n", start_x, start_y, start_z);
		printf("end %d %d %d\n", end_x, end_y, end_z);
		printf("\n");

		for (int x = start_x; x < end_x; x++) {
		for (int y = start_y; y < end_y; y++) {
		for (int z = start_z; z < end_z; z++) {

			chunk* chunk = player_chunk;
			world_chunk_pos chunk_pos;

			// dont check collision outside of block range
			if (y < 0 || y > WORLD_CHUNK_HEIGHT)
				continue;

			if (x - player_chunk_pos.x * WORLD_CHUNK_WIDTH >= WORLD_CHUNK_WIDTH)
				chunk_pos.x = player_chunk_pos.x + 1;
			else if (x - player_chunk_pos.x * WORLD_CHUNK_WIDTH < 0)
				chunk_pos.x = player_chunk_pos.x - 1;
			else
				chunk_pos.x = player_chunk_pos.x;

			if (z - player_chunk_pos.z * WORLD_CHUNK_WIDTH >= WORLD_CHUNK_WIDTH)
				chunk_pos.z = player_chunk_pos.z + 1;
			else if (z - player_chunk_pos.z * WORLD_CHUNK_WIDTH < 0)
				chunk_pos.z = player_chunk_pos.z - 1;
			else
				chunk_pos.z = player_chunk_pos.z;

			if (
					chunk_pos.x != player_chunk_pos.x ||
					chunk_pos.z != player_chunk_pos.z
			   ) {

				// check adjacent chunk
				chunk = world_chunk_lookup(chunk_pos);

				// dont check empty chunks for collision
				if (chunk == NULL) {
					fprintf(stderr, "WARNING: Player is entering an unloded chunk\n");
					continue;
				}
			}

			// adjust xyz to be relative to chunk
			// done this way due to how % handles negative numbers
			// this is otherwise the same as `x % WORLD_CHUNK_WIDTH`
			int cx = ((x % WORLD_CHUNK_WIDTH) + WORLD_CHUNK_WIDTH) % WORLD_CHUNK_WIDTH;
			int cy = y;
			int cz = ((z % WORLD_CHUNK_WIDTH) + WORLD_CHUNK_WIDTH) % WORLD_CHUNK_WIDTH;

			Vector3 block_pos = get_block_real_pos(chunk_pos, cx, cy, cz);

			// dont check air blocks
			if (chunk->blocks[cx][cy][cz].id == 0)
				continue;

			BoundingBox box = {
				.max = block_pos,
				.min = (Vector3){
					block_pos.x - 1,
					block_pos.y - 1,
					block_pos.z - 1,
				},
			};

			RayCollision collision = entity_block_collision_swept(player->e, box);

			if (collision.hit && collision.distance < nearest_collision.distance)
				nearest_collision = collision;
		}}}

		// collision response
		if (nearest_collision.hit && nearest_collision.distance <= Vector3Length(player->e.velocity) * delta_t) {
			// slide
			Vector3 norm = nearest_collision.normal;
			norm.x = norm.x == 0;
			norm.y = norm.y == 0;
			norm.z = norm.z == 0;

			player->e.velocity = Vector3Multiply(player->e.velocity, norm);

			if (nearest_collision.normal.y == 1)
				player->is_on_ground = 1;
		}
	}
}

static void player_physics(player* player) {
	const float delta_t = GetFrameTime();

	// Friction
	if (!Vector3Equals(player->e.velocity, Vector3Zero())) {
		if (player->is_on_ground)
			player_add_force(player, Vector3Scale(player->e.velocity, -GROUND_FRICTION));
		else
			player_add_force(player, Vector3Scale(player->e.velocity, -AIR_FRICTION));
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
			player_add_force(player, (Vector3){.y = g});
		} 

		// Collision
		player->is_on_ground = 0;
		player_block_collision(player);
	}

	// apply velocity to position, MUST BE LAST
	player_add_position(player, Vector3Scale(player->e.velocity, delta_t));
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
	}
}
