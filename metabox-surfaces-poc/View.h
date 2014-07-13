#pragma once

struct View {
	float x, tx;
	float y, ty;
	float scale, tscale;
	float angle, tangle;

	View() : x(0), tx(0), y(0), ty(0), scale(1), tscale(1), angle(0), tangle(0) {}
};