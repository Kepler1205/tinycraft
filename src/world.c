#include <stdlib.h>
#include <stdio.h>

#include <raylib.h>
#include <raymath.h>

#include "chunk.h"
#include "global.h"
#include "world.h"

world_data WORLD = {0};

void world_init(world_data* wd) {
	if (wd == NULL) {
		WORLD = (world_data){
			.chunk_opts = (chunk_generation_options){
				.seed = 42,
				.perlin_amplitude = 3.0f,
				.perlin_frequency = 0.05f,
				.octaves = 2,
			},
			.chunk_dict = (chunk_dictionary){0},
		};
	} else
		WORLD = *wd;
}

/* Get the real position of a block from chunk_pos and 
 * the offsets within that chunk.
 */
inline Vector3 get_block_real_pos(world_chunk_pos chunk_pos, int bx, int by, int bz) {
	return (Vector3){
		bx + 1 + chunk_pos.x * WORLD_CHUNK_WIDTH,
		by,
		bz + 1 + chunk_pos.z * WORLD_CHUNK_WIDTH,
	};
}

inline block* world_get_block(world_chunk_pos chunk_pos, uint16_t bx, uint16_t by, uint16_t bz) {
	chunk* chunk = world_chunk_lookup(chunk_pos);

	if (chunk == NULL)
		return NULL;

	return &chunk->blocks[bx][by][bz];
}

/* checks world dictionary for a chunk in pos.
 * returns NULL if no chunk exists in dictionary
 */
inline chunk* world_chunk_lookup(world_chunk_pos pos) {

	chunk_dict_entry* res = chunk_dict_lookup(&WORLD.chunk_dict, pos);
	if (res)
		return res->value;
	else
		return NULL;
}

chunk* world_load_chunk(world_chunk_pos pos) {
	// TODO: if chunk exists, load it from disk

	// if chunk does not exist yet, generate a new one
	if (world_chunk_lookup(pos) != NULL)
		return NULL;

	return chunk_generate_chunk(&WORLD.chunk_opts, &WORLD.chunk_dict, pos);
}

void world_unload_chunk(world_chunk_pos pos) {

	// TODO save chunk to disk

	chunk_dict_delete(&WORLD.chunk_dict, pos);
}

inline void world_unload_all_chunks(void) {
	chunk_dict_delete_all(&WORLD.chunk_dict);
}

static void render_chunk_border_walls(world_chunk_pos pos) {

	Color col = YELLOW;
	float fposx = pos.x * WORLD_CHUNK_WIDTH;
	float fposz = pos.z * WORLD_CHUNK_WIDTH;

	// horizontal lines
	for (unsigned int i = 0; i < WORLD_CHUNK_HEIGHT; i += 4) {
		DrawLine3D(
				(Vector3){fposx, i, fposz},
				(Vector3){fposx, i, fposz + WORLD_CHUNK_WIDTH},
				col);
		DrawLine3D(
				(Vector3){fposx, i, fposz},
				(Vector3){fposx + WORLD_CHUNK_WIDTH, i, fposz},
				col);
		DrawLine3D(
				(Vector3){fposx + WORLD_CHUNK_WIDTH, i, fposz + WORLD_CHUNK_WIDTH},
				(Vector3){fposx, i, fposz + WORLD_CHUNK_WIDTH},
				col);
		DrawLine3D(
				(Vector3){fposx + WORLD_CHUNK_WIDTH, i, fposz + WORLD_CHUNK_WIDTH},
				(Vector3){fposx + WORLD_CHUNK_WIDTH, i, fposz},
				col);
	}

	// vertical lines
	for (unsigned int i = 0; i < WORLD_CHUNK_WIDTH; i += 4) {
		DrawLine3D(
				(Vector3){fposx + i, WORLD_CHUNK_HEIGHT, fposz},
				(Vector3){fposx + i, 0, fposz},
				col);
		DrawLine3D(
				(Vector3){fposx + i, WORLD_CHUNK_HEIGHT, fposz + WORLD_CHUNK_WIDTH},
				(Vector3){fposx + i, 0, fposz + WORLD_CHUNK_WIDTH},
				col);
		DrawLine3D(
				(Vector3){fposx, WORLD_CHUNK_HEIGHT, fposz + i},
				(Vector3){fposx, 0, fposz + i},
				col);
		DrawLine3D(
				(Vector3){fposx + WORLD_CHUNK_WIDTH, WORLD_CHUNK_HEIGHT, fposz + i},
				(Vector3){fposx + WORLD_CHUNK_WIDTH, 0, fposz + i},
				col);
	}

}

void world_render_chunks(Camera3D* camera, Shader shader) {
	if (camera == NULL) {
		fprintf(stderr, "%s:%d Cannot render for NULL camera\n", __FILE__, __LINE__);
		return;
	}

	for (size_t i = 0; i < CHUNK_DICT_ENTRIES; i++) {
		chunk_dict_entry* entry = WORLD.chunk_dict.entries[i];

		while (entry != NULL) {
			// toggle chunk borders
			if (IsKeyPressed(KEY_F9)) {
				SETTINGS.show_chunk_borders ^= 0x1;
			}

			if (SETTINGS.show_chunk_borders) {
				world_chunk_pos pos = entry->key;

				// draw chunk borders
				DrawLine3D(
						(Vector3){pos.x * WORLD_CHUNK_WIDTH, 0, pos.z * WORLD_CHUNK_WIDTH}, 
						(Vector3){pos.x * WORLD_CHUNK_WIDTH,WORLD_CHUNK_HEIGHT, pos.z * WORLD_CHUNK_WIDTH}, 
						RED);

				if (
						camera->position.x >= pos.x * WORLD_CHUNK_WIDTH &&
						camera->position.x <  pos.x * WORLD_CHUNK_WIDTH + WORLD_CHUNK_WIDTH &&
						camera->position.z >= pos.z * WORLD_CHUNK_WIDTH &&
						camera->position.z <  pos.z * WORLD_CHUNK_WIDTH + WORLD_CHUNK_WIDTH
				   )
					render_chunk_border_walls(pos);
			}

			// render chunk
			chunk_render_chunk(entry->key, entry->value, camera, shader);
			entry = entry->next;
		}
	}
}

// ENTITY COLLISION

aabb_collision_result entity_block_collision(entity* e, Vector3 block_pos) {
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

/* swept AABB collision
 *
 * https://gamedev.net/tutorials/programming/general-and-gameplay-programming/swept-aabb-collision-detection-and-response-r3084/
 */
RayCollision entity_block_collision_swept(entity e, BoundingBox b) {
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
