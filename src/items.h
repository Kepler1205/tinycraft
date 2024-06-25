#pragma once

typedef enum {
	ITEM_NONE = 0,
	ITEM_BLOCK,
	ITEM_ITEM,
} item_type;

typedef struct {
	unsigned int id;
	unsigned int count;
	item_type type;
} item; // inventory and hotbar item slots
