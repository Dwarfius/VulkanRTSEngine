#version 420

#define MAX_BONES 100

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uvs;
layout(location = 3) in ivec4 boneIndices;
layout(location = 4) in vec4 boneWeights;

out vec4 normalOut;
out vec2 uvsOut;

layout (std140, binding = 0) uniform ObjectMatricesAdapter
{
	mat4 Model;
	mat4 ModelView;
	mat4 MVP;
};

layout (std140, binding = 1) uniform SkeletonAdapter
{
	mat4 SkinningMatrix[MAX_BONES];
};

mat4 GetBoneSkinningMat(ivec4 aBoneIndices, vec4 aBoneWeights)
{
	mat4 skinnedMat = SkinningMatrix[aBoneIndices.x] * aBoneWeights.x
					+ SkinningMatrix[aBoneIndices.y] * aBoneWeights.y
					+ SkinningMatrix[aBoneIndices.z] * aBoneWeights.z
					+ SkinningMatrix[aBoneIndices.w] * aBoneWeights.w;
	return skinnedMat;
}

void main() 
{
	mat4 skinningMat = GetBoneSkinningMat(boneIndices, boneWeights);
    gl_Position = MVP * skinningMat * vec4(position, 1);
	normalOut = Model * skinningMat * vec4(normal, 0);
    uvsOut = uvs;
}
