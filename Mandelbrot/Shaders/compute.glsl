#version 430 core
layout (local_size_x = 8, local_size_y = 4, local_size_z = 1) in;
layout(rgba32f, binding = 0) uniform image2D imgOutput;

precision highp float;

uniform bool frameAccumulation;
uniform int frameAccumulationCount;

uniform bool parameterSpace;
uniform bool flipY;
uniform bool flipX;

uniform vec2 selectedPoints[2];
uniform vec2 offset;
uniform float areaWidth;
uniform float areaHeight;
uniform float pixelLength;

uniform int maxIter;
uniform vec3 gradient[1024];
uniform int gradientSize;
uniform float gradientSensitivity;

float random(inout uint state)
{
	state = state * 747796405u + 2891336453u;
	uint result = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
	result = (result >> 22u) ^ result;
	return result /  4294967295.0; // 2^32 - 1
}

float random(float left, float right, inout uint state)
{
	return left + (right - left) * random(state);
}

float map(float x, float a, float b, float c, float d)
{
	return c + (x - a) * (d - c) / (b - a);
}

vec2 multC(vec2 x, vec2 y)
{
	float a = x.x;
	float b = x.y;
	float c = y.x;
	float d = y.y;
	return vec2(a*c - b*d, a*d + b*c);
}

vec2 divC(vec2 x, vec2 y)
{
	float a = x.x;
	float b = x.y;
	float c = y.x;
	float d = y.y;
	return vec2(a*c + b*d, b*c - a*d) / (c*c + d*d);
}

vec2 conj(vec2 z)
{
	return vec2(z.x, -z.y);
}

void main()
{
    ivec2 texelCoord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(imgOutput);
    float x = float(texelCoord.x * 2 - size.x) / size.x;
    float y = float(texelCoord.y * 2 - size.y) / size.y;
	uint seed = texelCoord.x * size.y + texelCoord.y + frameAccumulationCount * 968824447u;

	vec2 z0, z;
	if (parameterSpace) // Modified so starting z can varies
	{
		// C parameter, starting as all points in grid
		float x0 = map(x, -1.0f, 1.0f, offset.x - areaWidth,  offset.x + areaWidth ) + pixelLength * random(-1.0f, 1.0f, seed);
		float y0 = map(y, -1.0f, 1.0f, offset.y - areaHeight, offset.y + areaHeight) + pixelLength * random(-1.0f, 1.0f, seed);
		z0 = vec2(x0, y0);
		// Z variable, starting as c
		z = selectedPoints[0];
	}
	else
	{
		// C, now acts as choosen starting point
		z0 = selectedPoints[0];
		// Z, all points in grid
		float zx = map(x, -1.0f, 1.0f, offset.x - areaWidth,  offset.x + areaWidth ) + pixelLength * random(-1.0f, 1.0f, seed);;
		float zy = map(y, -1.0f, 1.0f, offset.y - areaHeight, offset.y + areaHeight) + pixelLength * random(-1.0f, 1.0f, seed);;
		z = vec2(zx, zy);
	}

	if (flipX)
	{
		z.x *= -1.0f;
		z0.x *= -1.0f;
	}
	if (flipY)
	{
		z.y *= -1.0f;
		z0.y *= -1.0f;
	}

	float brightness = 0.0f;
	vec2 zp = vec2(0.0f);
	vec2 zt;

	int i = 0;
	while(i < maxIter)
	{
		// Mandelbrot
		// z = multC(z, z) + z0;

		// Tricorn
		// z = conj(z);
		// z = multC(z, z) + z0;

		// Burning ship
		// z = abs(z);
		// z = multC(z, z) + z0;

		// HPDZ Buffalo
		// z = abs(z);
		// z = multC(z, z) - z + z0;

		// Lambda fractal
		// z = multC(z0, multC(z, vec2(1.0f - z.x, -z.y)));

		// Phoenix
		zt = z;
		z = multC(z, z) + z0 + multC(selectedPoints[1], zp);
		zp = zt;

		if (z.x * z.x + z.y * z.y > 500)
		{
			float log_zn = log(z.x*z.x + z.y*z.y) / 2.0f;
			float nu = log(log_zn / log(2.0f)) / log(2.0f);
			brightness = float(i) + 1.0f - nu;
			break;
		}
		i++;
	}

	int index = int(sqrt(brightness) * gradientSensitivity * gradientSize) % gradientSize;
	vec4 color = i == maxIter ? vec4(0.0f) : vec4(gradient[index], 1.0f);

	if (frameAccumulation)
		color = (imageLoad(imgOutput, texelCoord) * (frameAccumulationCount - 1) + color) / frameAccumulationCount;

	imageStore(imgOutput, texelCoord, color);
}