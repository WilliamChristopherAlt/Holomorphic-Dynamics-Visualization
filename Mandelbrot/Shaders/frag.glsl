#version 430 core
out vec4 FragColor;
uniform sampler2D screen;
in vec2 fragPos;
void main()
{
    vec2 uv = vec2(fragPos.x * 0.5f + 0.5f, fragPos.y * 0.5f + 0.5f);
	FragColor = texture(screen, uv);
}