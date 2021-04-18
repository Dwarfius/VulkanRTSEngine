#version 420

in vec3 colorOut;

layout(location = 0) out vec4 outColor;

void main() 
{
    outColor = vec4(colorOut, 1);
}