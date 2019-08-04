#version 420
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in DataIn
{
	vec2 TexCoords;
	vec4 TessLevel; // left, bottom, right, top
	float Height;
	flat int CheckerInd;
};
layout(location = 0) out vec4 outColor;

float getCompMax(vec4 v)
{
	return max(v.x, max(v.y, max(v.z, v.w)));
}

void main() 
{
	outColor.r = CheckerInd;
	outColor.g = getCompMax(TessLevel) / 64.f;
	outColor.b = Height < 10.f ? 1.f : 0.f;
    outColor.a = 1.f;
}