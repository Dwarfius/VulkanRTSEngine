#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 normalOut;
layout(location = 1) in vec2 uv;

layout(set = 1, binding = 1) uniform sampler2D tex;

layout(location = 0) out vec4 outColor;

void main() 
{
    outColor = vec4(normalOut, 1);
}