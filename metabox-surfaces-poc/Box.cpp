#include "Box.h"
#include "settings.h"
#include <Box2D/Box2D.h>

Box::Box() {
	parent = 0;
	texture = 0;
	body = 0;
	world = 0;

	for (int i = 0; i < 4; i++) {
		doors[i] = 0;
		body_edges[i] = 0;
	}

	for (int sx = 0; sx < BOX_SLOTS; sx++)
	for (int sy = 0; sy < BOX_SLOTS; sy++) {
		blocks[sx][sy] = 0;
	}
}