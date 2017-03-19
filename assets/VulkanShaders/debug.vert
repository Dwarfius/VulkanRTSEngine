#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uvs;
layout(location = 2) in vec3 normal;

out vec3 normalOut;
out vec2 uvsOut;

layout(binding = 0) uniform MatUBO {
    mat4 model;
    mat4 mvp;
} matrices;

void main()
{
	gl_Position = matrices.mvp * vec4(position, 1.0);
	normalOut = normalize(matrices.model * vec4(normal,0)).xyz;
	uvsOut = uvs;
}