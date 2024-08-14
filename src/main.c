#include <stdlib.h>
#include <stdio.h>

#include <raylib.h>
#include <raymath.h>

#define RLIGHTS_IMPLEMENTATION
#include <rlights.h>

#include "player.h"
#include "global.h"
#include "world.h"
#include "chunk.h"
 
int main(void) {
	// Window opts
	InitWindow(1920, 1080, "Tinycraft");
	SetTargetFPS(256);
	// MaximizeWindow();
	SetExitKey(KEY_BACKSPACE);
	DisableCursor();

	const int main_monitor = 0;
	const Vector2 m_pos = GetMonitorPosition(main_monitor);

	SetWindowPosition(m_pos.x, m_pos.y);
	SetWindowState(FLAG_WINDOW_MAXIMIZED | FLAG_WINDOW_RESIZABLE);

	// initializes settings and global variables, looking to deprecate;
	// settings should be their own translation unit
	globals_init();
	world_init(NULL);

	// initialize seed value
	perlin_noise_init(WORLD.chunk_opts.seed);

	player player = player_init();

	// shader stuff
	Shader chunk_shader = LoadShader("./shaders/chunk_vert.glsl", "./shaders/chunk_frag.glsl");
	if (chunk_shader.id == 0) {
		fprintf(stderr, "ERROR: Failed to load chunk shaders\n");
		return -1;
	}

	// Get shader locations
    chunk_shader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(chunk_shader, "mvp");
    chunk_shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(chunk_shader, "viewPos");
    chunk_shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocationAttrib(chunk_shader, "instanceTransform");

    // Set shader ambient light level
    int ambient_loc = GetShaderLocation(chunk_shader, "ambient");
    SetShaderValue(chunk_shader, ambient_loc, (float[4]){ 0.2f, 0.2f, 0.2f, 1.0f }, SHADER_UNIFORM_VEC4);

	// sunlight
	CreateLight(LIGHT_DIRECTIONAL, (Vector3){30,30,30}, Vector3Zero(), WHITE, chunk_shader);

	// ----- GAME LOOP ----- //
	while (!WindowShouldClose()) {

		ClearBackground(BLACK);

		// CHUNK GENERATION
		world_chunk_pos player_chunk_pos = {
			// use of floor() is required since truncation rounds
			// in th opposite direction for negative numbers.
			.x = floorf(player.e.position.x / WORLD_CHUNK_WIDTH),
			.z = floorf(player.e.position.z / WORLD_CHUNK_WIDTH),
		};

		/* // TEST
		world_load_chunk((world_chunk_pos){0}); */

		int rd = SETTINGS.render_distance;

		for (int i = -rd + 1; i < rd; i++) {
			for (int j = -rd + 1; j < rd; j++) {
				world_chunk_pos chunk_pos = {
					.x = i + player_chunk_pos.x,
					.z = j + player_chunk_pos.z,
				};

				world_load_chunk(chunk_pos);
			}
		}

		player_update(&player);

		// RENDER
		BeginDrawing();
	
		BeginMode3D(*player.camera);

		// draw player hitbox
		DrawCubeWiresV((Vector3){
					.x = player.e.position.x,
					.y = player.e.position.y + (player.e.size.y / 2),
					.z = player.e.position.z,
				}, player.e.size, WHITE);

		DrawCube((Vector3){ // player pos box
				.x = player.e.position.x,
				.y = player.e.position.y + .05,
				.z = player.e.position.z
				}, player.e.size.x, .1, player.e.size.z, PURPLE);

		// DrawGrid(32, 1);
		DrawGrid(50, 16);
		world_render_chunks(player.camera, chunk_shader);

		EndMode3D();

		// DRAW UI
		
		int screen_center_x = GetScreenWidth() / 2;
		int screen_center_y = GetScreenHeight() / 2;
		int reticle_width = 3;
		int reticle_height = 60;
		DrawRectangle(
				screen_center_x - reticle_width / 2,
				screen_center_y - reticle_height / 2,
				reticle_width,
				reticle_height,
				(Color) { .a=128, .r=20,.g=20,.b=20 }
				);
		DrawRectangle(
				screen_center_x - reticle_height / 2,
				screen_center_y - reticle_width / 2,
				reticle_height,
				reticle_width,
				(Color) { .a=128, .r=20,.g=20,.b=20 }
				);

		// Draw menu
		switch (player.gamemode) {
			case (MODE_PAUSED):
				// DRAW PAUSE MENU
				break;
			case (MODE_MENU):
			default:
				break;
		}

		DrawFPS(0,0);

		const char* mode_str;
		switch (player.gamemode) {
			case (MODE_SURVIVAL):
				mode_str = "SURVIVAL\n";
				break;
			case (MODE_CREATIVE):
				mode_str = "CREATIVE\n";
				break;
			case (MODE_SPECTATOR):
				mode_str = "SPECTATOR\n";
				break;
			default:
				break;
		}

		DrawText(mode_str, 15, 30, 22, RED);

		char buf[512];
		snprintf(buf, sizeof(buf), 
				"player.e.position: %f %f %f\n\n"
				"player.e.velocity: %f %f %f\n\n"
				"player chunk pos: %d %d\n\n"
				"Player is flying: %d\n\n"
				"Player is on ground: %d\n\n"
				"Camera position: %f %f %f\n\n"
				"Camera up: %f %f %f\n\n"
				"Camera target: %f %f %f\n\n"
				"Camera pos/target dist: %f\n\n"
				,
				player.e.position.x, player.e.position.y, player.e.position.z,
				player.e.velocity.x, player.e.velocity.y, player.e.velocity.z,
				player_chunk_pos.x, player_chunk_pos.z,
				player.is_flying,
				player.is_on_ground,
				player.camera->position.x, player.camera->position.y, player.camera->position.z,
				player.camera->up.x, player.camera->up.y, player.camera->up.z,
				player.camera->target.x, player.camera->target.y, player.camera->target.z,
				Vector3Distance(player.camera->position, player.camera->target)
				);
		DrawText(buf, 15, 50, 22, ORANGE);
	
		EndDrawing();
	}

	UnloadShader(chunk_shader);
	world_unload_all_chunks();
	player_destroy(&player);
	CloseWindow();

	return 0;
}

