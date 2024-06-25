#include <stdio.h>

#include <raylib.h>
#include <raymath.h>
#include <raygui.h>

#include "menu.h"
#include "player.h"
#include "global.h"

int main(void) {

	// Window opts
	InitWindow(1920, 1080, "Tinycraft");
	SetTargetFPS(165);
	// MaximizeWindow();
	SetExitKey(KEY_BACKSPACE);
	DisableCursor();

	const int main_monitor = 0;
	const Vector2 m_pos = GetMonitorPosition(main_monitor);

	SetWindowPosition(m_pos.x, m_pos.y);
	SetWindowState(
			  FLAG_WINDOW_MAXIMIZED 
			| FLAG_WINDOW_RESIZABLE
			);
	
	bool cursor_status = 0;

	globals_init(); // initializes settings and global variables
	Font gui_font = GuiGetFont();
	gui_font.baseSize *= game_settings.gui_scale;
	GuiSetFont(gui_font);

	player player = player_init();

	typedef enum {
		MODE_SURVIVAL,
		MODE_CREATIVE,
		MODE_PAUSED,
		MODE_MENU,
		MODE_TITLESCREEN,
	} gamemode_type;
	gamemode_type gamemode = MODE_SURVIVAL; // default gamemode

	while (!WindowShouldClose()) {
		ClearBackground(BLACK);

		if (IsKeyPressed(KEY_ESCAPE)) {
			switch (gamemode) {
				case (MODE_MENU):
				case (MODE_PAUSED):
					gamemode = MODE_SURVIVAL;
					break;
				default:
					gamemode = MODE_PAUSED;
			}
		}

		if (IsKeyPressed(KEY_E)) {
			switch (gamemode) {
				case (MODE_MENU):
				case (MODE_PAUSED):
					gamemode = MODE_SURVIVAL;
					break;
				default:
					gamemode = MODE_MENU;
			}
		}


		// INPUT
		Vector3 previous_position = player.position;
		if (gamemode == MODE_MENU || gamemode == MODE_PAUSED) {
			if (!cursor_status) {
				EnableCursor();
				cursor_status = 1;
			}

			if (gamemode == MODE_MENU)
				player_physics(&player);

		} else {
			if (cursor_status) {
				DisableCursor();
				cursor_status = 0;
			}

			if (gamemode == MODE_CREATIVE)
				player.is_flying = 1; // TODO setup player.gamemode 

			player_movement(&player);
			player_physics(&player);
		}

		// COLLISION

/*
		if (player.position.y < 0) {
			player.position.y = 0;
		}
*/

		// RENDER
		BeginDrawing();
	
		BeginMode3D(*player.camera);

		// draw player outline
		DrawCubeWires((Vector3){
				.x = player.position.x,
				.y = player.position.y + 1,
				.z = player.position.z
				}, 1, 2, 0.6, RED);

		DrawCube((Vector3){ // player pos box
				.x = player.position.x,
				.y = player.position.y + .25,
				.z = player.position.z
				}, .5, .5, .5, PURPLE);

		DrawCubeV((Vector3){ // player target box
				.x = player.camera->target.x,
				.y = player.camera->target.y,
				.z = player.camera->target.z
				}, (Vector3){.25,.25,.25}, PURPLE);

		DrawCube((Vector3){0,1,0},
				1.0f, 1.0f, 1.0f,
				(Color){
					.a = 255,
					.r = 0,
					.g = 128,
					.b = 255,
				});


		DrawPlane(Vector3Zero(), (Vector2){20,20}, GRAY);
		DrawGrid(20, 1);

		EndMode3D();

		// DRAW UI


		// Draw menu
		switch (gamemode) {
			case (MODE_PAUSED):
				menu_pause_draw();
				break;
			case (MODE_MENU):
			default:
				break;
		}

		DrawFPS(0,0);

		char buf[512];
		snprintf(buf, sizeof(buf), 
				"Player position: %f %f %f\n\n"
				"Player velocity: %f %f %f\n\n"
				"Player inputvec: %f %f %f\n\n"
				"Player is flying: %d\n\n"
				"Player is on ground: %d\n\n"
				"Camera position: %f %f %f\n\n"
				"Camera up: %f %f %f\n\n"
				"Camera target: %f %f %f\n\n"
				"Camera pos/target dist: %f\n\n",
				player.position.x, player.position.y, player.position.z,
				player.velocity.x, player.velocity.y, player.velocity.z,
				player.input_vector.x, player.input_vector.y, player.input_vector.z,
				player.is_flying,
				player.is_on_ground,
				player.camera->position.x, player.camera->position.y, player.camera->position.z,
				player.camera->up.x, player.camera->up.y, player.camera->up.z,
				player.camera->target.x, player.camera->target.y, player.camera->target.z,
				Vector3Distance(player.camera->position, player.camera->target)
				);
		DrawText(buf, 15, 15, 22, ORANGE);
		// printf("v = %f %f %f\n", player.velocity.x, player.velocity.y, player.velocity.z);
	
		EndDrawing();
	}

	player_destroy(&player);
	CloseWindow();

	return 0;
}

