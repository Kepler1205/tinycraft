#include <raylib.h>

#include "global.h"

void globals_init(void) {
	Vector2 screen_resolution = {
		.x = GetScreenWidth(),
		.y = GetScreenHeight(),
	};
	game_settings = (settings){
		.display_resolution = screen_resolution,
		.gui_scale = screen_resolution.y / 100,
	};
}
