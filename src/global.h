#pragma once

#include <raylib.h>

typedef struct {
	int gui_scale;
	Vector2 display_resolution;
} settings;

static settings game_settings;

void globals_init(void);
