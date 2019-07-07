#version 420
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in DataIn
{
	vec2 TexCoords;
	flat int TessLevel;
	float DistFromCenter;
	flat int CheckerInd;
};
layout(location = 0) out vec4 outColor;

void main() 
{
	outColor.r = CheckerInd;
	outColor.g = TessLevel / 64.f;
	outColor.b = 0.f;
    outColor.a = 1.f;
}