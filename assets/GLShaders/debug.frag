#version 420

uniform sampler2D tex;

in vec3 normalOut;
in vec2 uvsOut;

layout(location = 0) out vec4 outColor;

void main() 
{
    outColor = vec4(normalOut, 1);
}