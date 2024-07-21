#include <raylib.h>

#include "global.h"

settings SETTINGS = {0};

void globals_init(void) {
	Vector2 screen_resolution = {
		.x = GetScreenWidth(),
		.y = GetScreenHeight(),
	};

	SETTINGS = (settings){
		.render_distance = 4,
		.display_resolution = screen_resolution,
		.gui_scale = screen_resolution.y / 100,
		.show_chunk_borders = true,
	};
}
