#pragma once

#include <inttypes.h>

#include <raylib.h>

#define WORLD_MAX_HEIGHT 256 // max world height in blocks

// block position in world space
typedef struct {
	int64_t x, y, z;
} world_block_pos;

typedef struct {
	int id;
	world_block_pos position;
} block;

// used to locate a chunk
typedef struct {
	int32_t x, y, z;
} world_chunk_pos;

typedef struct {
	world_chunk_pos position;
	block blocks[16][WORLD_MAX_HEIGHT][16];
} chunk;

// entities are moveable objects with one collider 
typedef struct {
	int id;
	Vector3 position; // location at the center of the bottom face
	Vector3 velocity;
	Vector3 size;
} entity;

// CONVERSION FUNCTIONS
Vector3 world_block_pos_to_real(world_block_pos input);
world_block_pos real_pos_to_world_block(Vector3 input);

// COLLISION FUNCTIONS

