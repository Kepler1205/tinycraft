#pragma once

#include <raylib.h>
#include "world.h"

// entities are moveable objects/characters with an aabb collider 
typedef struct {
	int id;
	Vector3 position; // location at the center of the bottom face
	Vector3 velocity;
	Vector3 size;
	float mass;
	bool is_on_ground;
} entity;

// COLLISION FUNCTIONS

/* Calculates collision for an entity and block.
 * returns a signed value for overlap depth so you can
 * just do e.position + result.overlap_depth to move the 
 * entity out of the block
 */
aabb_collision_result entity_aabb(entity* e, Vector3 block_pos);

RayCollision entity_aabb_swept(entity e, BoundingBox b);

/* Handler for entity/block collision in world
 */
void entity_block_collision(entity* e);



