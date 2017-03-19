#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform MatUBO {
    mat4 Model;
    mat4 mvp;
} matrices;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUv;
layout(location = 2) in vec3 normal;

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) out vec3 normalOut;
layout(location = 1) out vec2 uv;

void main() {
    gl_Position = matrices.mvp * vec4(inPos, 1.0);
    normalOut = normalize(matrices.Model * vec4(normal,0)).xyz;
    uv = inUv;
}