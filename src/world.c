#include <raylib.h>
#include <raymath.h>

#include "world.h"

Vector3 world_block_pos_to_real(world_block_pos input) {
	return (Vector3){
		.x = (float)input.x,
		.y = (float)input.y,
		.z = (float)input.z,
	};
}

// truncates input
world_block_pos real_pos_to_world_block(Vector3 input) {
	return (world_block_pos){
		.x = (int64_t)input.x,
		.y = (int64_t)input.y,
		.z = (int64_t)input.z,
	};
}

void entity_block_collision(entity* e, world_block_pos block_pos) {
	Vector3 b_min = world_block_pos_to_real(block_pos);
	Vector3 b_max = Vector3Add(b_min, Vector3One());

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

	bool overlap_x = (e_min.x < b_max.x) && (e_max.x > b_min.x);
	bool overlap_y = (e_min.y < b_max.y) && (e_max.y > b_min.y);
	bool overlap_z = (e_min.z < b_max.z) && (e_max.z > b_min.z);

	if (overlap_x && overlap_y && overlap_z) {
		/* collision found, now determine which face
		 * has been collided with 
		 */
/* #define MIN(x, y) (x) < (y) ? (x) : (y)
#define MAX(x, y) (x) > (y) ? (x) : (y)

		Vector3 overlap_depth = {
			.x = MIN(e_max.x, b_max.x) - MAX(e_min.x, b_min.x),
			.y = MIN(e_max.y, b_max.y) - MAX(e_min.y, b_min.y),
			.z = MIN(e_max.z, b_max.z) - MAX(e_min.z, b_min.z),
		};

		float smallest_overlap = MIN(MIN(overlap_depth.x, overlap_depth.y), overlap_depth.z);

#undef MAX
#undef MIN

		if (smallest_overlap == overlap_depth.x) {
			if (e_max.x < b_max.x) {
				// right of e and left of b
				e->velocity.x = 0;
			} else {
				// left of e and right of b
			}
		} */

		// if (e->position.x <)
	}
}
