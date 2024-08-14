#include <stdlib.h>
#include <stdio.h>

#include <raylib.h>
#include <raymath.h>

#include "world.h"
#include "entity.h"

aabb_collision_result entity_aabb(entity* e, Vector3 block_pos) {
	Vector3 b_max = block_pos;
	(void)block_pos;
	Vector3 b_min = {
		b_max.x - 1,
		b_max.y - 1,
		b_max.z - 1,
	};

	aabb_collision_result result = {0};

	Vector3 e_min = {
		e->position.x - (e->size.x / 2),
		e->position.y,
		e->position.z - (e->size.z / 2),
	};
	Vector3 e_max = {
		e->position.x + (e->size.x / 2),
		e->position.y + e->size.y,
		e->position.z + (e->size.z / 2),
	};

	Vector3 overlap_depth = {
		fminf(e_max.x, b_max.x) - fmaxf(e_min.x, b_min.x),
		fminf(e_max.y, b_max.y) - fmaxf(e_min.y, b_min.y),
		fminf(e_max.z, b_max.z) - fmaxf(e_min.z, b_min.z),
	};

	if ((overlap_depth.x > 0) && (overlap_depth.y > 0) && (overlap_depth.z > 0)) {
		/* collision found, now determine which face
		 * has been collided with 
		 */
		result.collided = 1;

		float smallest_overlap = fminf(overlap_depth.x, fminf(overlap_depth.y, overlap_depth.z));

		if (overlap_depth.x == smallest_overlap) {
			overlap_depth.y = 0;
			overlap_depth.z = 0;
			if (e_max.x < b_max.x)
				overlap_depth.x = -overlap_depth.x;
		} else if (overlap_depth.y == smallest_overlap) {
			overlap_depth.x = 0;
			overlap_depth.z = 0;
			if (e_max.y < b_max.y)
				overlap_depth.y = -overlap_depth.y;
		} else {
			overlap_depth.x = 0;
			overlap_depth.y = 0;
			if (e_max.z < b_max.z)
				overlap_depth.z = -overlap_depth.z;
		}

		result.collision_depth = overlap_depth;
	}

	return result;
}

/* Swept aabb collision detection
 */
RayCollision entity_aabb_swept(entity e, BoundingBox b) {
	// add entity size to box size
	b.max.x += e.size.x * 0.5f;
	b.max.y += e.size.y * 0.5f;
	b.max.z += e.size.z * 0.5f;

	b.min.x -= e.size.x * 0.5f;
	b.min.y -= e.size.y * 0.5f;
	b.min.z -= e.size.z * 0.5f;

	// point ray in direction of movement
	Ray ray = {
		.position = Vector3Add(e.position, (Vector3){.y = e.size.y * 0.5f}),
		.direction = Vector3Normalize(e.velocity),
	};

	// let raylib do the rest
	return GetRayCollisionBox(ray, b);
}

void entity_block_collision(entity* e) {
	e->is_on_ground = 0;

	// collision has to be checked 3 times
	// once for each axis
	for (unsigned int axis = 0; axis < 3; axis++) {

		// chunk that the player is inside
		world_chunk_pos player_chunk_pos = { 
			floorf(e->position.x / WORLD_CHUNK_WIDTH),
			floorf(e->position.z / WORLD_CHUNK_WIDTH),
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
		Vector3 frame_travel = Vector3Scale(e->velocity, delta_t);

		// broadphase box
		BoundingBox bphase = {
			.max = Vector3Add(e->position, frame_travel),
			.min = e->position,
		};

		const float padding = 0.0f;

		// adjustments to include entire entity in
		// current and next frame
		if (e->velocity.x > 0) {
			bphase.min.x -= (e->size.x + padding) * 0.5f;
			bphase.max.x += (e->size.x + padding) * 0.5f;
		} else {
			bphase.min.x += (e->size.x + padding) * 0.5f;
			bphase.max.x -= (e->size.x + padding) * 0.5f;
		}

		if (e->velocity.y > 0) {
			bphase.max.y += (e->size.y + padding);
		} else {
			bphase.min.y += (e->size.y + padding);
		}

		if (e->velocity.z > 0) {
			bphase.min.z -= (e->size.z + padding) * 0.5f;
			bphase.max.z += (e->size.z + padding) * 0.5f;
		} else {
			bphase.min.z += (e->size.z + padding) * 0.5f;
			bphase.max.z -= (e->size.z + padding) * 0.5f;
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

			RayCollision collision = entity_aabb_swept(*e, box);

			if (collision.hit && collision.distance < nearest_collision.distance)
				nearest_collision = collision;
		}}}

		// collision response
		if (nearest_collision.hit && nearest_collision.distance <= Vector3Length(e->velocity) * delta_t) {
			// slide
			Vector3 norm = nearest_collision.normal;
			norm.x = norm.x == 0;
			norm.y = norm.y == 0;
			norm.z = norm.z == 0;

			e->velocity = Vector3Multiply(e->velocity, norm);

			if (nearest_collision.normal.y == 1)
				e->is_on_ground = 1;
		}
	}
}
