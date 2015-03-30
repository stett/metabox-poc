#include "Entity.h"
#include "Box.h"

Entity::Entity() {
    body = 0;
    container = 0;
}

Entity::~Entity() {
    if (container)
        container->entities.remove(this);
}

void Entity::set_container(shared_ptr<Box> box, b2Vec2 position, b2Vec2 velocity) {
    //
    if (container)
        container->entities.remove(this);

    //
    auto world = box->recursive ? box->parent->world : box->world;

    // Generate the new player body
    generate_body(world, position, velocity);

    // Set the player container
    container = (box ? (box->recursive ? box->parent : box) : 0);
}

void Entity::generate_body(shared_ptr<b2World> world, b2Vec2 position, b2Vec2 velocity) {

    // Remove the old body if there was one
    if (body)
        body->GetWorld()->DestroyBody(body);

    // Make a new body for the player in the new world
    float size = (float)BOX_PHYSICAL_SIZE / (float)BOX_SLOTS;
    b2BodyDef body_def;
    body_def.type = b2BodyType::b2_dynamicBody;
    body_def.userData = this;
    body_def.position = position;
    body_def.linearVelocity = velocity;
    body = world->CreateBody(&body_def);
    body->SetFixedRotation(true);

    // Create a rectangular fixture for him
    b2PolygonShape shape;
    shape.SetAsBox(size * .3f, size * .3f);
    auto fixture = body->CreateFixture(&shape, 1);
    fixture->SetFriction(FRICTION);
    b2Filter filter;
    filter.categoryBits = B2_CAT_MAIN;
    filter.maskBits = B2_CAT_MAIN;
    fixture->SetFilterData(filter);
}