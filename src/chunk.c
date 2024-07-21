#include <stdlib.h>
#include <stdio.h>

#include <raylib.h>
#include <raymath.h>

#include "chunk.h"
#include "global.h"

// CHUNK DICTIONARY
static size_t chunk_dict_hash(world_chunk_pos key) {
	return abs(key.z + 1) * 5 + abs(key.x) ;
}

chunk_dict_entry* chunk_dict_lookup(chunk_dictionary* chunk_dict, world_chunk_pos key) {
	size_t index = chunk_dict_hash(key) % CHUNK_DICT_ENTRIES;
	chunk_dict_entry* entry = chunk_dict->entries[index];

	while (entry != NULL) {
		if (entry->key.x == key.x && entry->key.z == key.z)
			return entry;
		entry = entry->next;
	}

	// chunk not found
	return NULL;
}

void chunk_dict_delete(chunk_dictionary* chunk_dict, world_chunk_pos key) {
	size_t index = chunk_dict_hash(key) % CHUNK_DICT_ENTRIES;
	chunk_dict->count--;
	chunk_dict_entry* entry = chunk_dict->entries[index];
	chunk_dict_entry* prev = NULL;

	while (entry != NULL && entry->key.x != key.x && entry->key.z != key.z) {
		prev = entry;
		entry = entry->next;
	}

	if (entry == NULL)
		return;

	// entry has been found

	if (prev == NULL)
		chunk_dict->entries[index] = entry->next;
	else
		prev->next = entry->next;

	free(entry->value);
	free(entry);
}

void chunk_dict_delete_all(chunk_dictionary* chunk_dict) {
	chunk_dict->count = 0;
	for (size_t i = 0; i < CHUNK_DICT_ENTRIES; i++) {
		chunk_dict_entry* entry = chunk_dict->entries[i];
		if (entry != NULL) {
			while (entry->next != NULL) {
				chunk_dict_entry* next_entry = entry->next;

				UnloadMesh(entry->value->face_mesh);
				free(entry->value->transforms);
				free(entry->value);
				free(entry);

				entry = next_entry;
			}
		}
	}
}

void chunk_dict_insert(chunk_dictionary* chunk_dict, world_chunk_pos key, chunk* value) {
	size_t index = chunk_dict_hash(key) % CHUNK_DICT_ENTRIES;

	chunk_dict->count++;
	chunk_dict_entry* entry = malloc(sizeof(chunk_dict_entry));

	if (entry == NULL) {
		fputs("Failed to allocate memory for a chunk entry\n", stderr);
		return;
	}

	*entry = (chunk_dict_entry){
		.key = key,
		.value = value,
		.next = NULL,
	};
	
	if (chunk_dict->entries[index] == NULL)
		chunk_dict->entries[index] = entry;
	else {
		chunk_dict_entry* entry_ptr = chunk_dict->entries[index];

		while (entry_ptr->next != NULL)
			entry_ptr = entry_ptr->next;

		entry_ptr->next = entry;
	}
}

void for_each_chunk(chunk_dictionary* chunk_dict, void (*func)(world_chunk_pos pos, chunk* chunk, void* args), void* args) {
	for (size_t i = 0; i < CHUNK_DICT_ENTRIES; i++) {
		chunk_dict_entry* entry = chunk_dict->entries[i];
		while (entry != NULL) {
			func(entry->key, entry->value, args);
			entry = entry->next;
		}
	}
}

void for_each_block(chunk* chunk, void (*func)(block* block, void* args), void* args) {
	for (unsigned int x = 0; x < WORLD_CHUNK_WIDTH; x++) {
		for (unsigned int y = 0; y < WORLD_CHUNK_HEIGHT; y++) {
			for (unsigned int z = 0; z < WORLD_CHUNK_WIDTH; z++) {
				
				func(&chunk->blocks[x][y][z], args);
			}
		}
	}
}


#include <math.h>

// PERLIN NOISE GENERATION

// permutation table for perlin noise
static int ptable[512];

void perlin_noise_init(unsigned int seed) {
	int permutation[] = { 151,160,137,91,90,15,
				131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
				190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
				88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
				77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
				102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
				135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
				5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
				223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
				129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
				251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
				49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
				138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180 };

	srand(seed);

	// shuffle permutation array with seed
	for (int i = 255; i > 0; i--) {
		int j = rand() % (i + 1);
		int temp = permutation[i];
		permutation[i] = permutation[j];
		permutation[j] = temp;
	}

	// fill ptable with permutation values repeated twice
	for (int i = 0; i < 256; i++)
		ptable[256+i] = ptable[i] = permutation[i];
}

static inline float perlin_fade(float t) {
	return t * t * t * (t * (t * 6 - 15) + 10);
}

static inline float perlin_lerp(float t, float a, float b) {
	return a + t * (b - a);
}

static inline float perlin_grad(int hash, float x, float y, float z) {
	int h = hash & 15;
	float u = h < 8 ? x : y;
	float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
	return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

float perlin_noise_3D(float x, float y, float z) {
	int X = (int)floor(x) & 255;
	int Y = (int)floor(y) & 255;
	int Z = (int)floor(z) & 255;

	x -= floor(x);
	y -= floor(y);
	z -= floor(z);

	float u = perlin_fade(x);
	float v = perlin_fade(y);
	float w = perlin_fade(z);

	int A = ptable[X] + Y;
	int AA = ptable[A] + Z;
	int AB = ptable[A + 1] + Z;
	int B = ptable[X + 1] + Y;
	int BA = ptable[B] + Z;
	int BB = ptable[B + 1] + Z;

	return perlin_lerp(w, 
			perlin_lerp(v,
				perlin_lerp(u, perlin_grad(ptable[AA], x, y, z), perlin_grad(ptable[BA], x - 1, y, z)),
			perlin_lerp(u, perlin_grad(ptable[AB], x, y - 1, z), perlin_grad(ptable[BB], x - 1, y - 1, z))),
			perlin_lerp(v,
				perlin_lerp(u, perlin_grad(ptable[AA + 1], x, y, z - 1), perlin_grad(ptable[BA + 1], x - 1, y, z - 1)),
				perlin_lerp(u, perlin_grad(ptable[AB + 1], x, y - 1, z - 1), perlin_grad(ptable[BB + 1], x - 1, y - 1, z - 1))));
}

float perlin_noise_2D(float x, float y) {
	return perlin_noise_3D(x, y, 0.0f);
}

// calculate perlin noise value based on chunk_generation_options
static float chunk_perlin_noise(chunk_generation_options* opts, float x, float z) {
				float value = 0;

				float amplitude = opts->perlin_amplitude;
				float frequency = opts->perlin_frequency;

				for (unsigned int i = 0; i < opts->octaves; i++) {
					value += amplitude * perlin_noise_2D(
							x * frequency + .5f, 
							z * frequency + .5f
							);

					frequency *= 2;
					amplitude /= 2;
				}

				// clip value between -1 and 1
				value = value > 1.0f ? 1.0f : value;
				value = value < -1.0f ? -1.0f : value;

				// normalise for positive numbers
				// float noise_normalized = (noise_val / (max_val * 2)) + 1;
				const float max_val = 10;
				const float min_val = 5;
				return min_val + (value + 1.0f) * 0.5f * max_val;
}

#include "world.h"

#include <string.h>
#include <rlgl.h>

void print_matrix(Matrix m) {
		printf( " %.2f %.2f %.2f %.2f \n"
				" %.2f %.2f %.2f %.2f \n"
				" %.2f %.2f %.2f %.2f \n"
				" %.2f %.2f %.2f %.2f \n",
				m.m0, m.m1, m.m2, m.m3,
				m.m4, m.m5, m.m6, m.m7,
				m.m8, m.m9, m.m10, m.m11,
				m.m12, m.m13, m.m14, m.m15
				);
}

// CHUNK GENERATION

// unless you intend to re-generate the chunk, use world_load_chunk
chunk* chunk_generate_chunk(chunk_generation_options* opts, chunk_dictionary* chunk_dict, world_chunk_pos pos) {
	chunk* chunk = malloc(sizeof(*chunk));
	if (chunk == NULL) {
		fprintf(stderr, "Failed to allocate memory for chunk: %d, %d", pos.x, pos.z);
		return NULL;
	}

	for (unsigned int x = 0; x < WORLD_CHUNK_WIDTH; x++) {
	for (unsigned int y = 0; y < WORLD_CHUNK_HEIGHT; y++) {
	for (unsigned int z = 0; z < WORLD_CHUNK_WIDTH; z++) {

		Vector3 block_pos = get_block_real_pos(pos, x, y, z);
		float noise = chunk_perlin_noise(opts, block_pos.x, block_pos.z);

		// truncate
		unsigned int height = floorf(noise);

		if (y == height) {
			chunk->blocks[x][y][z].id = 1; // GRASS
		} else if (y > height) {
			chunk->blocks[x][y][z].id = 0; // AIR
		} else {
			chunk->blocks[x][y][z].id = 2; // STONE
		}
	}}}

	// get adjacent chunk block id
	block adjacent_blocks[4][WORLD_CHUNK_WIDTH][WORLD_CHUNK_HEIGHT];

	/* adjacent_blocks[n] layout
	 *
	 *  +y  +z
	 *  |  / 
	 *  |/___ +x
	 *             0
	 *        ___________
	 *      /           /
	 * 3  /  CHUNK    /  2
	 *  /           /
	 * ------------
	 *     1
	 */
	for (unsigned int i = 0; i < 4; i++) {
	for (unsigned int x = 0; x < WORLD_CHUNK_WIDTH; x++) {
	for (unsigned int y = 0; y < WORLD_CHUNK_HEIGHT; y++) {

		Vector3 block_pos;

		switch (i) {
			case (0): // +z
				block_pos = get_block_real_pos(pos, x, y, WORLD_CHUNK_WIDTH);
				break;
			case (1): // -z
				block_pos = get_block_real_pos(pos, x, y, -1);
				break;
			case (2): // +x
				block_pos = get_block_real_pos(pos, WORLD_CHUNK_WIDTH, y, x);
				break;
			case (3): // -x
				block_pos = get_block_real_pos(pos, -1, y, x);
				break;
		}

		float noise = chunk_perlin_noise(opts, block_pos.x, block_pos.z);
		unsigned int height = floorf(noise);

		if (y == height) {
			adjacent_blocks[i][x][y].id = 1;
		} else if (y > height) {
			adjacent_blocks[i][x][y].id = 0;
		} else {
			adjacent_blocks[i][x][y].id = 2;
		}
	}}}

	unsigned int face_count = 0;
	Matrix* transforms = malloc(sizeof(Matrix) * 6 * (WORLD_CHUNK_WIDTH * WORLD_CHUNK_WIDTH * WORLD_CHUNK_HEIGHT)/2);

	// generate instance data for chunk
	for (unsigned int x = 0; x < WORLD_CHUNK_WIDTH; x++) {
	for (unsigned int y = 0; y < WORLD_CHUNK_HEIGHT; y++) {
	for (unsigned int z = 0; z < WORLD_CHUNK_WIDTH; z++) {
		unsigned int block_id = chunk->blocks[x][y][z].id;
		if (block_id == 0)
			continue;

#define FACE_FRONT  (1 << 0) // +z
#define FACE_BACK   (1 << 1) // -z
#define FACE_LEFT   (1 << 2) // +x
#define FACE_RIGHT  (1 << 3) // -x
#define FACE_TOP    (1 << 4) // +y
#define FACE_BOTTOM (1 << 5) // -y

		unsigned char faces = 0;

		switch (x) {
			case (WORLD_CHUNK_WIDTH - 1):
				faces |= adjacent_blocks[2][z][y].id == 0 ? FACE_LEFT   : 0;
				faces |= chunk->blocks[x-1][y][z].id == 0 ? FACE_RIGHT  : 0;
				break;
			case (0):
				faces |= adjacent_blocks[3][z][y].id == 0 ? FACE_RIGHT  : 0;
				faces |= chunk->blocks[x+1][y][z].id == 0 ? FACE_LEFT   : 0;
				break;
			default:
				faces |= chunk->blocks[x+1][y][z].id == 0 ? FACE_LEFT   : 0;
				faces |= chunk->blocks[x-1][y][z].id == 0 ? FACE_RIGHT  : 0;
		}

		switch (y) {
			case (WORLD_CHUNK_HEIGHT - 1):
				faces |= chunk->blocks[x][y-1][z].id == 0 ? FACE_BOTTOM : 0;
				break;
			case (0):
				faces |= chunk->blocks[x][y+1][z].id == 0 ? FACE_TOP    : 0;
				break;
			default:
				faces |= chunk->blocks[x][y+1][z].id == 0 ? FACE_TOP    : 0;
				faces |= chunk->blocks[x][y-1][z].id == 0 ? FACE_BOTTOM : 0;
		}

		switch (z) {
			case (WORLD_CHUNK_WIDTH - 1):
				faces |= adjacent_blocks[0][x][y].id == 0 ? FACE_FRONT  : 0;
				faces |= chunk->blocks[x][y][z-1].id == 0 ? FACE_BACK   : 0;
				break;
			case (0):
				faces |= adjacent_blocks[1][x][y].id == 0 ? FACE_BACK   : 0;
				faces |= chunk->blocks[x][y][z+1].id == 0 ? FACE_FRONT  : 0;
				break;
			default:
				faces |= chunk->blocks[x][y][z+1].id == 0 ? FACE_FRONT  : 0;
				faces |= chunk->blocks[x][y][z-1].id == 0 ? FACE_BACK   : 0;
		}

		if (faces == 0)
			continue;

		// translate the top face to the cube faces
		Matrix t, r, res;

		if (faces & FACE_FRONT) {
			t = MatrixTranslate(x + .5, y - .5, z + 1);
			r = MatrixRotateX(PI/2);
			res = MatrixMultiply(r, t);
			memcpy(transforms + face_count++, &res, sizeof(Matrix));
		}
		
		if (faces & FACE_BACK) {
			t = MatrixTranslate(x + .5, y - .5, z);
			r = MatrixRotateX(-PI/2);
			res = MatrixMultiply(r, t);
			memcpy(transforms + face_count++, &res, sizeof(Matrix));
		}
		
		if (faces & FACE_LEFT) {
			t = MatrixTranslate(x + 1, y - .5, z + .5);
			r = MatrixRotateZ(-PI/2);
			res = MatrixMultiply(r, t);
			memcpy(transforms + face_count++, &res, sizeof(Matrix));
		}

		if (faces & FACE_RIGHT) {
			t = MatrixTranslate(x, y - .5, z + .5);
			r = MatrixRotateZ(PI/2);
			res = MatrixMultiply(r, t);
			memcpy(transforms + face_count++, &res, sizeof(Matrix));
		}

		if (faces & FACE_TOP) {
			res = MatrixTranslate(x + .5, y, z + .5);
			memcpy(transforms + face_count++, &res, sizeof(Matrix));
		}

		if (faces & FACE_BOTTOM) {
			t = MatrixTranslate(x + .5, y - 1, z + .5);
			r = MatrixRotateX(PI);
			res = MatrixMultiply(r, t);
			memcpy(transforms + face_count++, &res, sizeof(Matrix));
		}
	}}}

	// shrink allocation to fit data
	transforms = realloc(transforms, face_count * sizeof(Matrix));

	for (unsigned int i = 0; i < face_count; i++)
		transforms[i] = MatrixMultiply(transforms[i], MatrixTranslate(pos.x * WORLD_CHUNK_WIDTH, 0, pos.z * WORLD_CHUNK_WIDTH));

	chunk->face_count = face_count;
	chunk->transforms = transforms;
	chunk->face_mesh = GenMeshPlane(1,1,1,1);

	chunk_dict_insert(chunk_dict, pos, chunk);

	return chunk;
}

static bool is_box_in_frustum(Vector4 planes[6], Vector3 min, Vector3 max) {
	for (unsigned int i = 0; i < 6; i++) {
		Vector4 p = planes[i];

		Vector3 positive = {
			(p.x >= 0) ? max.x : min.x,
			(p.y >= 0) ? max.y : min.y,
			(p.z >= 0) ? max.z : min.z,
		};

		if ((
			p.x * positive.x + 
			p.y * positive.y + 
			p.z * positive.z +
			p.w
			) < 0)
			return false; // box is outside of the frustum
	}
	return true; // box is inside or intesects the frustum
}

static void create_frustum_planes(Matrix viewp_matrix, Vector4* fplanes) {

	// left plane
	fplanes[0].x = viewp_matrix.m3  + viewp_matrix.m0;
	fplanes[0].y = viewp_matrix.m7  + viewp_matrix.m4;
	fplanes[0].z = viewp_matrix.m11 + viewp_matrix.m8;
	fplanes[0].w = viewp_matrix.m15 + viewp_matrix.m12;

	// right plane
	fplanes[1].x = viewp_matrix.m3  - viewp_matrix.m0;
	fplanes[1].y = viewp_matrix.m7  - viewp_matrix.m4;
	fplanes[1].z = viewp_matrix.m11 - viewp_matrix.m8;
	fplanes[1].w = viewp_matrix.m15 - viewp_matrix.m12;

	// top plane
	fplanes[2].x = viewp_matrix.m3  - viewp_matrix.m1;
	fplanes[2].y = viewp_matrix.m7  - viewp_matrix.m5;
	fplanes[2].z = viewp_matrix.m11 - viewp_matrix.m9;
	fplanes[2].w = viewp_matrix.m15 - viewp_matrix.m13;

	// bottom plane
	fplanes[3].x = viewp_matrix.m3  + viewp_matrix.m1;
	fplanes[3].y = viewp_matrix.m7  + viewp_matrix.m5;
	fplanes[3].z = viewp_matrix.m11 + viewp_matrix.m9;
	fplanes[3].w = viewp_matrix.m15 + viewp_matrix.m13;

	// near plane
	fplanes[4].x = viewp_matrix.m3  + viewp_matrix.m2;
	fplanes[4].y = viewp_matrix.m7  + viewp_matrix.m6;
	fplanes[4].z = viewp_matrix.m11 + viewp_matrix.m10;
	fplanes[4].w = viewp_matrix.m15 + viewp_matrix.m14;

	// far plane
	fplanes[5].x = viewp_matrix.m3  - viewp_matrix.m2;
	fplanes[5].y = viewp_matrix.m7  - viewp_matrix.m6;
	fplanes[5].z = viewp_matrix.m11 - viewp_matrix.m10;
	fplanes[5].w = viewp_matrix.m15 - viewp_matrix.m14;

	// normalise frustum planes
	for (unsigned int i = 0; i < 6; i++) {
		// Quaternions are Vector4s so this is acceptable
		QuaternionNormalize(fplanes[i]);
	}
}

// CHUNK RENDERING
void chunk_render_chunk(world_chunk_pos pos, chunk* chunk, Camera3D* camera, Shader shader) {
	if (chunk == NULL) {
		fprintf(stderr, "%s:%d render NULL chunk (%d, %d)\n", __FILE__, __LINE__, pos.x, pos.z);
		return;
	}

	// frustum culling initialization
	Matrix projection_matrix = MatrixPerspective(
			camera->fovy * DEG2RAD,
			(float)GetScreenWidth() / (float)GetScreenHeight(), 
			0.01f, 1000.0f);
	Matrix viewp_matrix = MatrixMultiply(GetCameraMatrix(*camera), projection_matrix);

	// create frustum planes
	Vector4 fplanes[6];

	create_frustum_planes(viewp_matrix, fplanes);

	// occlude chunk if outside the frustum
	if (!is_box_in_frustum(fplanes, 
				(Vector3){
					pos.x * WORLD_CHUNK_WIDTH,
					0,
					pos.z * WORLD_CHUNK_WIDTH
				}, (Vector3){
					pos.x * WORLD_CHUNK_WIDTH + WORLD_CHUNK_WIDTH,
					WORLD_CHUNK_HEIGHT,
					pos.z * WORLD_CHUNK_WIDTH + WORLD_CHUNK_WIDTH
				})) {
		return;
	}

	float cam_pos[3] = {camera->position.x, camera->position.y, camera->position.z};
	SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], cam_pos, SHADER_UNIFORM_VEC3);
	// SetShaderValueMatrix(shader, shader.locs[SHADER_LOC_MATRIX_MVP], projection_matrix);

	// draw chunks

	// do the equivilent of LoadMaterialDefault without heap allocation
	Material mat = DEFAULT_MATERIAL;

	mat.shader = shader;
	mat.maps[MATERIAL_MAP_DIFFUSE].color = BLUE;
	mat.maps[MATERIAL_MAP_NORMAL].value = .2f;
	DrawMeshInstanced(chunk->face_mesh, mat, chunk->transforms, chunk->face_count);
}
