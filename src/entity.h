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

// COLLISION

/* Calculates collision for an entity and block.
 * returns a signed value for overlap depth so you can
 * just do e.position + result.overlap_depth to move the 
 * entity out of the block
 */
bool entity_aabb(entity* e, Vector3 block_pos, Vector3* collision_depth);

RayCollision entity_aabb_swept(entity e, BoundingBox b);

/* Handler for entity/block collision in world
 */
void entity_block_collision(entity* e);

// PHYSICS

/* called per-frame to add a force vector
 */
void entity_add_force(entity* e, Vector3 force);

/* called once to add an impulse (instantaneous force)
 */
void entity_add_impulse(entity* e, Vector3 force);
