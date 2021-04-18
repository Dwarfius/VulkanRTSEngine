#version 420

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

uniform mat4 vp;

out vec3 colorOut;

void main()
{
	gl_Position = vp * vec4(position, 1.0);
	colorOut = color;
}