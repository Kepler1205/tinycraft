#pragma once

#include <raylib.h>

#define WORLD_CHUNK_HEIGHT 256
#define WORLD_CHUNK_WIDTH 16

typedef struct {
	unsigned int id;
} block;

// used to locate a chunk with chunk_dict
typedef struct {
	int x, z;
} world_chunk_pos;

// data format for vertex buffer
typedef struct {
	Vector3 v0,v1,v2,v3;
	Color color;
} vb_format;

typedef struct {
	Vector3 position;
	Vector3 normal;
	Color color;
} chunk_vertex;

typedef struct {
	// vertex array object
	// unsigned int vao;

	unsigned int vao;
	unsigned int face_count;
	Mesh face_mesh;
	Matrix* transforms;
	block blocks[WORLD_CHUNK_WIDTH][WORLD_CHUNK_HEIGHT][WORLD_CHUNK_WIDTH];
} chunk;

typedef struct {
	unsigned int seed;
	float perlin_amplitude;
	float perlin_frequency;
	unsigned int octaves;
} chunk_generation_options;

typedef struct chunk_dict_entry chunk_dict_entry;
struct chunk_dict_entry {
	world_chunk_pos key;
	chunk* value;
	chunk_dict_entry* next;
};

/* Ideally should be more than the number of chunks
 * loaded at one time to avoid collisions 
 */
#define CHUNK_DICT_ENTRIES 128
typedef struct {
	chunk_dict_entry* entries[CHUNK_DICT_ENTRIES];
	unsigned int count;
} chunk_dictionary;

// DICTIONARY FUNCTIONS

/* Lookup an entry with the key
 */
chunk_dict_entry* chunk_dict_lookup(chunk_dictionary* chunk_dict, world_chunk_pos key);

/* Free an entry from the dictionary, along with the chunk
 * data itself.
 */
void chunk_dict_delete(chunk_dictionary* chunk_dict, world_chunk_pos key);

/* Free the entire dictionary, along with the chunks
 * themselves
 */
void chunk_dict_delete_all(chunk_dictionary* chunk_dict);

/* Make an entry into the chunk dictionary
 */
void chunk_dict_insert(chunk_dictionary* chunk_dict, world_chunk_pos key, chunk* value);

/* Runs func for each loaded chunk in no particular order.
 */
void for_each_chunk(chunk_dictionary* chunk_dict, void (*func)(world_chunk_pos pos, chunk* chunk, void* args), void* args);

/* Runs func for each block in the chunk
 */
void for_each_block(chunk* chunk, void (*func)(block* block, void* args), void* args);

// GENERATION FUNCITONS
chunk* chunk_generate_chunk(chunk_generation_options* chunk_opts, chunk_dictionary* chunk_dict, world_chunk_pos pos);
void chunk_render_chunk(world_chunk_pos pos, chunk* chunk, Camera3D* camera, Shader shader);

/* Initialize perlin noise with an integer seed.
 * This is required before getting a value from
 * the perlin noise functions.
 * There is no corresponding destroy function.
 */
void perlin_noise_init(unsigned int seed);

/* Get the value of perlin noise at (x, y, z)
 */
float perlin_noise_3D(float x, float y, float z);

/* Get the value of perlin noise at (x, y)
 */
float perlin_noise_2D(float x, float y);
