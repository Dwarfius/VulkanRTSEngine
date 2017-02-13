#version 420

uniform sampler2D tex;

in vec4 normalOut;
in vec2 uvsOut;

layout(location = 0) out vec4 outColor;

void main() 
{
    outColor = vec4(uvsOut, 0, 1);
}