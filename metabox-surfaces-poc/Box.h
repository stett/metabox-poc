#ifndef _BOX_H_
#define _BOX_H_

#include "settings.h"
#include <Box2D/Box2D.h>
#include <SFML/Graphics.hpp>
#include <memory>
#include <list>
using namespace std;

enum BoxFace { Top = 0, Right, Bottom, Left };
enum BoxState {
	Gridded = 0,	// Moveable, but snaps to target grid slot
	Fixed,			// Imoveable, fixed to grid slot
	Free			// Free floating physics body
};

class Box;

class Slot {
public:
	int x, y;
	Box* parent;
	shared_ptr<Box> child;

    bool edges(BoxFace face) {
        return ((face == Left && x == 0) || (face == Right && x == BOX_SLOTS - 1) ||
                (face == Top && y == 0) || (face == Bottom && y == BOX_SLOTS - 1));
    }
};

class BoxDoor {
public:
	shared_ptr<Box> box;
    BoxFace face;
    Slot* slot;
	bool open;
	float t;

    shared_ptr<BoxDoor> adjacency;
    shared_ptr<BoxDoor> shingle_up;
    shared_ptr<BoxDoor> shingle_down;

	BoxDoor(shared_ptr<Box> _box, BoxFace _face, bool _open, Slot* _slot) : box(_box), face(_face), open(_open), slot(_slot), t(1) {}
};

class Box {
public:
	int id;
	shared_ptr<Box> parent;
	list<shared_ptr<Box>> children;
	shared_ptr<sf::RenderTexture> texture;
	shared_ptr<b2World> world;
	sf::Texture* bg;
	sf::Texture* fg;
	b2Body* body;
	b2Body* world_edges;
	b2Fixture* body_edges[4];
	shared_ptr<BoxDoor> doors[4];
	BoxState state;
	Slot slots[BOX_SLOTS][BOX_SLOTS];
	Slot* slot;
	Slot* target_slot;
	int sx;
	int sy;
	int target_sx;
	int target_sy;
	int blocks[BOX_SLOTS][BOX_SLOTS];
	bool recursive;

	Box();
	//~Box();
};

#endif