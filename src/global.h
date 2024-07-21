#pragma once

#include <raylib.h>

typedef struct {
	unsigned int render_distance;
	int gui_scale;
	Vector2 display_resolution;
	bool show_chunk_borders;
} settings;

extern settings SETTINGS;


void globals_init(void);
