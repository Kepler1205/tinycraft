#include <raylib.h>
#include <raygui.h>

#include "menu.h"
#include "global.h"

void menu_pause_draw(void) {
	Vector2 screen_center = {
		.x = GetScreenWidth(),
		.y = GetScreenHeight(),
	};

	Vector2 label_size = {
		.x = 100,
		.y = 60,
	};

	GuiWindowBox((Rectangle){
			.x = screen_center.x / 2 - label_size.x / 2,
			.y = screen_center.y / 2 - label_size.y / 2,
			.width = label_size.x,
			.height = label_size.y,
			},
		"Test gui");
}
