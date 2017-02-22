#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform MatUBO {
    mat4 model;
    mat4 mvp;
} matrices;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUv;
layout(location = 2) in vec3 normal;

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) out vec4 fragColor;

void main() {
    //gl_Position = matrices.mvp * vec4(inPos, 1.0);
    gl_Position = vec4(inPos, 1);
    fragColor = vec4(1, 1, 1, 1);
}