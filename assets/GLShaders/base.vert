#version 420

//layout(location = 0) in vec2 position;
//layout(location = 1) in vec3 color;

vec2 position[3] = vec2[3](
		vec2(0.0, -0.5), vec2(0.5, 0.5), vec2(-0.5, 0.5)
	);
vec3 color[3] = vec3[3](
		vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1)
	);

out vec3 fragColor;

void main() {
    gl_Position = vec4(position[gl_VertexID], 0.0, 1.0);
    fragColor = color[gl_VertexID];
}
