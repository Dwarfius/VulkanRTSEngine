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

layout(location = 0) out vec3 fragColor;

vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

void main() {
    //gl_Position = matrices.mvp * vec4(inPos, 1.0);
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = vec4(1, 1, 1, 1);
}