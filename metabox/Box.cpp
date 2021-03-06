#include "Box.h"
#include "settings.h"
#include <Box2D/Box2D.h>


Box::Box() {
	parent = 0;
	texture = 0;
	body = 0;
    world = 0;
    recursive = false;
    world_edges = 0;
    slot = 0;

    // Initialize doors and physics body edges
	for (int i = 0; i < 4; i++) {
		doors[i] = 0;
		body_edges[i] = 0;
	}

    // Initialize blocks & slots
	for (int sx = 0; sx < BOX_SLOTS; sx++)
	for (int sy = 0; sy < BOX_SLOTS; sy++) {
		blocks[sx][sy] = 0;
	}

    // Initialize slots
    for (int sx = 0; sx < BOX_SLOTS; sx++)
    for (int sy = 0; sy < BOX_SLOTS; sy++) {
        auto slot = &slots[sx][sy];
        slot->parent = this;
        slot->x = sx;
        slot->y = sy;
    }
}