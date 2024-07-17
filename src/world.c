#include <stdlib.h>
#include <stdio.h>

#include <raylib.h>
#include <raymath.h>

#include "chunk.h"
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
Vector3 get_block_real_pos(world_chunk_pos chunk_pos, int bx, int by, int bz) {
	return (Vector3){
		.x = bx + chunk_pos.x * WORLD_CHUNK_WIDTH,
		.y = by,
		.z = bz + chunk_pos.z * WORLD_CHUNK_WIDTH,
	};
}

block* world_get_block(world_chunk_pos chunk_pos, uint16_t bx, uint16_t by, uint16_t bz) {
	chunk* chunk = world_chunk_lookup(chunk_pos);

	if (chunk == NULL)
		return NULL;

	return &chunk->blocks[bx][by][bz];
}

/* checks world dictionary for a chunk in pos.
 * returns NULL if no chunk exists in dictionary
 */
chunk* world_chunk_lookup(world_chunk_pos pos) {

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

void world_unload_all_chunks(void) {
	chunk_dict_delete_all(&WORLD.chunk_dict);
}

void world_render_chunks(Camera3D* camera, Shader shader) {
	if (camera == NULL) {
		fprintf(stderr, "%s:%d Cannot render for NULL camera\n", __FILE__, __LINE__);
		return;
	}

	for (size_t i = 0; i < CHUNK_DICT_ENTRIES; i++) {
		chunk_dict_entry* entry = WORLD.chunk_dict.entries[i];

		while (entry != NULL) {
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
		.x = b_max.x - 1,
		.y = b_max.y - 1,
		.z = b_max.z - 1,
	};

	aabb_collision_result result = {0};

	Vector3 e_min = {
		.x = e->position.x - (e->size.x / 2),
		.y = e->position.y,
		.z = e->position.z - (e->size.z / 2),
	};
	Vector3 e_max = {
		.x = e->position.x + (e->size.x / 2),
		.y = e->position.y + e->size.y,
		.z = e->position.z + (e->size.z / 2),
	};

	/* makes entities sink in the block by a small value
	 * this fixes entities alternating between
	 * colliding and not colliding every frame
	 */
	const float sink_value = 0.001;

	Vector3 overlap_depth = {
		.x = fminf(e_max.x, b_max.x) - fmaxf(e_min.x, b_min.x) - sink_value,
		.y = fminf(e_max.y, b_max.y) - fmaxf(e_min.y, b_min.y) - sink_value,
		.z = fminf(e_max.z, b_max.z) - fmaxf(e_min.z, b_min.z) - sink_value,
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

return result;
	} else
		return result;
}
