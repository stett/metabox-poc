#ifndef _PLAYER_H_
#define _PLAYER_H_

#include "Box.h"
#include <box2d/Box2D.h>
#include <memory>
#include <stack>
using namespace std;

class Player {
public:
	b2Body* body;
	shared_ptr<Box> container;
	stack<shared_ptr<Box>> recursions;

	Player();
};

#endif