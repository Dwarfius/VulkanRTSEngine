#version 420

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uvs;
layout(location = 2) in vec3 normal;

out vec3 normalOut;
out vec2 uvsOut;

uniform mat4 Model;
uniform mat4 mvp;

void main() 
{
    gl_Position = mvp * vec4(position, 1.0);
    normalOut = normalize(Model * vec4(normal,0)).xyz;
    uvsOut = uvs;
}
