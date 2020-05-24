uniform sampler2D texture;
uniform float t;
uniform float face;
uniform float face_pos;
uniform float entropy;
uniform float seed;

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

float rand(vec2 seed) {
    return fract(sin(dot(seed.xy ,vec2(12.9898,78.233))) * 43758.5453);
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

	// Randomize the pixel a bit
	uv.x += 2.0f * (rand(vec2(pos.x, pos.y * seed)) - 0.5f) * .002f * entropy;
	uv.y += 2.0f * (rand(vec2(pos.y * seed, pos.x * seed)) - 0.5f) * .002f * entropy;

	// Set the pixel
	vec4 pixel = texture2D(texture, uv);

	// multiply it by the vertex color
	gl_FragColor = gl_Color * pixel;
}
