#version 420

layout (location = 0) in vec2 Position;
layout (location = 1) in vec2 UV;
layout (location = 2) in uint Color;

layout (std140, binding = 0) uniform ImGUIAdapter
{
	mat4 ProjMtx;
};

out vec2 Frag_UV;
out vec4 Frag_Color;

float GetColorComp(int i)
{
	const uint mask = 0xFFu << (8 * (3 - i));
	const uint component = (Color & mask) >> (8 * (3 - i));
	return component / 255.f;
}

void main()
{
    Frag_UV = UV;
    Frag_Color = vec4(GetColorComp(3), GetColorComp(2), GetColorComp(1), GetColorComp(0));
    gl_Position = ProjMtx * vec4(Position.xy,0,1);
}