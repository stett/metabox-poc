#ifndef _ENTITY_H_
#define _ENTITY_H_

#include <box2d/Box2D.h>
#include <memory>
#include <stack>
using std::shared_ptr;
using std::stack;

class Box;

class Entity {
public:
    b2Body* body;
    shared_ptr<Box> container;
    stack<shared_ptr<Box>> recursions;

    Entity();
    ~Entity();

    void set_container(shared_ptr<Box> box, b2Vec2 position, b2Vec2 velocity);
    void generate_body(shared_ptr<b2World> world, b2Vec2 position, b2Vec2 velocity);
};

#endif