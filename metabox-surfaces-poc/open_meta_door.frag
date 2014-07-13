uniform sampler2D texture;
uniform float t;
uniform float face;
uniform float face_pos;

float smoothstep(float a, float b, float t) {
	return a + (b - a) * t * t * (3 - 2 * t);
}

float expand_parallel_axis(float t, float x, float y, float y0) {
	return (2*y-3*t*t*x*x*(-3+2*x)*(-1+14*y0) + 2*t*t*t*x*x*(-3+2*x)*(-1+14*y0)) /
		   (2-36*t*t*x*x*(-3+2*x) + 24*t*t*t*x*x*(-3+2*x));
}

float expand_perpendicular_axis(float t, float x) {
	if (t < .0001f) return x;
	return (1 - 3*t*t + 2*t*t*t - sqrt((t-1)*(t-1)*(t-1)*(t-1) * (1+2*t)*(1+2*t) + 4*(3-2*t)*t*t*x)) /
		   (2*t*t*(-3+2*t));
}

void main(void)
{
	// Get the original uv coords
	vec2 pos = gl_TexCoord[0].xy;
	vec2 uv = pos;

	// Perform vertical mutation
	float scale = 7.0;
	
	// Top
	if (face == 0.0f) {
		float x0 = (face_pos + 0.5f) / scale;//(6.5f - face_pos) / scale;
		uv.x = expand_parallel_axis(t, pos.y, pos.x, x0);
		uv.y = expand_perpendicular_axis(t, pos.y);

	// Right
	} else if (face == 1.0f) {
		float y0 = (6.5f - face_pos) / scale;
		uv.y = expand_parallel_axis(t, pos.x, pos.y, y0);
		uv.x = expand_perpendicular_axis(t, pos.x);

	// Bottom
	} else if (face == 2.0f) {
		float x0 = (face_pos - 0.5f) / scale;
		uv.x = expand_parallel_axis(t, 1 - pos.y, pos.x, x0);
		uv.y = 1 - expand_perpendicular_axis(t, 1 - pos.y);

	// Left
	} else if (face == 3.0f) {
		float y0 = (face_pos - 0.5f) / scale;
		uv.y = expand_parallel_axis(t, 1 - pos.x, pos.y, y0);
		uv.x = 1 - expand_perpendicular_axis(t, 1 - pos.x);
	}


	// Set the pixel
	vec4 pixel = texture2D(texture, uv);

	// multiply it by the vertex color
	gl_FragColor = gl_Color * pixel;
}





/*
//float t = .9;

// Common values
float x2 = pos.x * pos.x;
float a = 3 - 2 * pos.x;
float b = 1 - x2 * a;

*/

//
// Vertical transform
//

// Above
/*if (pos.y < y0 * b) {
	uv.y = pos.y / b;

// Below
} else if (pos.y > y0 * b + x2 * a) {
	uv.y = (pos.y - x2 * a) / (1 - x2 * a);

// Inbetween
} else {
	uv.y = (pos.y - x2 * a * (-y0 * scale + 0.5)) / (1 - x2 * a * (-scale + 1));
}*/

//uv.y = (pos.y - x2 * a * (-y0 * scale + 0.5)) / (1 - x2 * a * (-scale + 1));


	

//
// Time dependence
//
/*
uv.x = smoothstep(pos.x, uv.x, t);
uv.y = smoothstep(pos.y, uv.y, t);
*/

/*
// Top
if (face == 0) {

// Right
} else if (face == 1) {
	float y0 = ((float)face_pos + (float)0.5) / scale;
	uv.y = (2*pos.y-3*t*t*pos.x*pos.x*(-3+2*pos.x)*(-1+14*y0) + 2*t*t*t*pos.x*pos.x*(-3+2*pos.x)*(-1+14*y0)) /
		    (2-36*t*t*pos.x*pos.x*(-3+2*pos.x) + 24*t*t*t*pos.x*pos.x*(-3+2*pos.x));

// Bottom
} else if (face == 2) {
		

// Left
} else if (face == 3) {
	float y0 = ((float)face_pos + 0.5) / scale;
	pos.x = 1 - pos.x
	uv.y = (2*pos.y-3*t*t*pos.x*pos.x*(-3+2*pos.x)*(-1+14*y0) + 2*t*t*t*pos.x*pos.x*(-3+2*pos.x)*(-1+14*y0)) /
		    (2-36*t*t*pos.x*pos.x*(-3+2*pos.x) + 24*t*t*t*pos.x*pos.x*(-3+2*pos.x));
}
*/