#pragma once

#include <stdint.h>
#include <raylib.h>

#include "chunk.h"

// entities are moveable objects with one collider 
typedef struct {
	int id;
	Vector3 position; // location at the center of the bottom face
	Vector3 velocity;
	Vector3 size;
	float mass;
} entity;

typedef struct {
	// resizeable array of pointers to chunks
	chunk_dictionary chunk_dict;
	chunk_generation_options chunk_opts;
} world_data;

/* Contains data relevent to rendering the world
 */
extern world_data WORLD;

typedef struct {
	Vector3 collision_depth;
	bool collided;
	float collision_time;
	Vector3 normal;
} aabb_collision_result;

/* Initializes WORLD using wd. 
 * If wd is NULL, the default values are used.
 * This must be called before any chunk
 * generation.
 * There is currently no "world_destroy" function.
 */
void world_init(world_data* wd);

// CONVERSION FUNCTIONS
Vector3 get_block_real_pos(world_chunk_pos chunk_pos, int bx, int by, int bz);

// CHUNK FUNCTIONS

/* checks world dictionary for a chunk in pos.
 * returns NULL if no chunk exists in dictionary
 */
chunk* world_chunk_lookup(world_chunk_pos position);

/* Will generate a chunk at pos if no chunk exists already
 */
chunk* world_load_chunk(world_chunk_pos pos);

/* Unloads a chunk at pos
 */
void world_unload_chunk(world_chunk_pos pos);

/* Unload every chunk in world
 */
void world_unload_all_chunks(void);

/* Get a block's handle from a loaded chunk.
 * ONLY USE FOR ONE-OFFS.
 * Ths function does a chunk lookup to get one block,
 * if you want to manipulate a lot of block data,
 * use world_chunk_lookup and keep the chunk* around.
 */
block* world_get_block(world_chunk_pos chunk_pos, uint16_t bx, uint16_t by, uint16_t bz);

/* Render all chunks in WORLD dictionary
 */
void world_render_chunks(Camera3D* camera, Shader shader);

// COLLISION FUNCTIONS

/* Calculates collision for an entity and block.
 * returns a signed value for overlap depth so you can
 * just do e.position + result.overlap_depth to move the 
 * entity out of the block
 */
aabb_collision_result entity_block_collision(entity* e, Vector3 block_pos);
RayCollision entity_block_collision_swept(entity e, BoundingBox b);
