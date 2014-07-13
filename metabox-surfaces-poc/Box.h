#ifndef _BOX_H_
#define _BOX_H_

#include "settings.h"
#include <Box2D/Box2D.h>
#include <SFML/Graphics.hpp>
#include <memory>
#include <list>
using namespace std;

enum BoxFace { Top = 0, Right, Bottom, Left };
enum BoxState { Gridded = 0, Fixed, Free };

class Box;

class Slot {
public:
	int x, y;
	shared_ptr<Box> parent;
	list<shared_ptr<Box>> children;
	shared_ptr<Slot> adj[4];
};

class BoxDoor {
public:
	shared_ptr<Box> parent;
	bool open;
	int sx;
	int sy;
	float t;
	BoxDoor(shared_ptr<Box> _parent, bool _open, int _sx, int _sy) : parent(_parent), open(_open), sx(_sx), sy(_sy), t(1) {};
};

class Box {
public:
	int id;
	shared_ptr<Box> parent;
	list<shared_ptr<Box>> children;
	shared_ptr<sf::RenderTexture> texture;
	shared_ptr<b2World> world;
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

	Box();
};

#endif