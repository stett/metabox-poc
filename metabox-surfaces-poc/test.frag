uniform sampler2D texture;

void main(void)
{
	// lookup the pixel in the texture
	vec2 uv = gl_TexCoord[0].xy;
	vec4 pixel = texture2D(texture, uv);

	// multiply it by the color
	gl_FragColor = gl_Color * pixel;
}