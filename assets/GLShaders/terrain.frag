#version 420
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in DataIn
{
	mediump vec2 TexCoords;
	flat int TessLevel;
};
layout(location = 0) out vec4 outColor;

void main() 
{
    outColor = mix(vec4(1, 0, 0, 1), vec4(0, 1, 0, 1), float(TessLevel) / 64);
}