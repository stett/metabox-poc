uniform sampler2D texture;
uniform float t;
uniform float face;
uniform float face_pos;
//uniform float entropy;
uniform float seed;

float smoothstep(float a, float b, float t) {
	return a + (b - a) * t * t * (3 - 2 * t);
}

float rand(vec2 seed) {
    return fract(sin(dot(seed.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

void main(void)
{
	// Get the original uv coords
	vec2 pos = gl_TexCoord[0].xy;
	vec2 uv = pos;

	// Randomize the pixel a bit
	//uv.x += 2.0f * (rand(vec2(pos.x, pos.y * seed)) - 0.5f) * .002f * entropy;
	//uv.y += 2.0f * (rand(vec2(pos.y * seed, pos.x * seed)) - 0.5f) * .002f * entropy;

	// Set the pixel
	vec4 pixel = texture2D(texture, uv);

	//
	float scale = 7.0;
	float pos_diff;
	if (face == 0.0)      pos_diff = pos.x * scale - face_pos;
	else if (face == 1.0) pos_diff = pos.y * scale - face_pos;
	else if (face == 2.0) pos_diff = (1 - pos.x) * scale - face_pos + 1;
	else if (face == 3.0) pos_diff = (1 - pos.y) * scale - face_pos + 1;

	if (0 < pos_diff && pos_diff < 1) {
		float alpha = (pos_diff - 0.5) * (pos_diff - 0.5) * 10 * rand(vec2(pos.x * seed, pos.y * seed)) * (1 - t);
		//float alpha = rand(vec2(pos.x, pos.y));
		gl_FragColor = vec4(1.0, 1.0, 1.0, alpha) * pixel;
	} else {
		gl_FragColor = gl_Color * pixel;
	}

	// multiply it by the vertex color
	//gl_FragColor = vec4(0.0, 1.0, 0.0, 0.3);//gl_Color * pixel;
}
