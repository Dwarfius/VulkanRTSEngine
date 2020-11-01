#version 420

#define MAX_BONES 100

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uvs;
layout(location = 2) in vec3 normal;

out vec3 normalOut;
out vec2 uvsOut;

layout (std140, binding = 0) uniform ObjectMatricesAdapter
{
	mat4 Model;
	mat4 ModelView;
	mat4 MVP;
};

struct Bone
{
	vec3 Pos;
	vec3 Scale;
	quat Rotation;
};

layout (std140, binding = 1) uniform SkeletonAdapter
{
	int BoneCount;
	vec3 Pos[MAX_BONES];
	vec3 Scale[MAX_BONES];
	quat Rotation[MAX_BONES];
};

void main() 
{
    gl_Position = MVP * vec4(position, 1.0);
    normalOut = normalize(Model * vec4(normal,0)).xyz;
    uvsOut = uvs;
}
