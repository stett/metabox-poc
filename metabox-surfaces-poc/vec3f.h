#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <Box2D/Box2D.h>
#include <math.h>

struct vec2f {

	// Members
	float x, y;

	// 'Tors
	vec2f(float x_ = 0.f, float y_ = 0.f) : x(x_), y(y_) {}
	vec2f(b2Vec2 b2_vec) : x(b2_vec.x), y(b2_vec.y) {}
	vec2f(sf::Vector2f sf_vec) : x(sf_vec.x), y(sf_vec.y) {}

	// Methods
	float length() {
		return sqrt(x * x + y * y);
	}

	// Overloaded operators
	vec2f operator -(vec2f &rhs) {
		return vec2f(x - rhs.x, y - rhs.y);
	}

	vec2f operator +(vec2f &rhs) {
		return vec2f(x + rhs.x, y + rhs.y);
	}
	
	vec2f operator *(float rhs) {
		return vec2f(x * rhs, y * rhs);
	}
};