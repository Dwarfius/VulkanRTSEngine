#version 420

#define MAX_BONES 100

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uvs;
layout(location = 2) in ivec2 boneIndices;
layout(location = 3) in vec2 boneWeights;

out vec3 normalOut;
out vec2 uvsOut;

layout (std140, binding = 0) uniform ObjectMatricesAdapter
{
	mat4 Model;
	mat4 ModelView;
	mat4 MVP;
};

layout (std140, binding = 1) uniform SkeletonAdapter
{
	vec3 Pos[MAX_BONES];
	vec3 Scale[MAX_BONES];
	vec4 Rotation[MAX_BONES];
};

vec4 QuatMultiplyQuat(vec4 p, vec4 q)
{
	// taken from GLM
	vec4 newQuat;
	newQuat.w = p.w * q.w - p.x * q.x - p.y * q.y - p.z * q.z;
	newQuat.x = p.w * q.x + p.x * q.w + p.y * q.z - p.z * q.y;
	newQuat.y = p.w * q.y + p.y * q.w + p.z * q.x - p.x * q.z;
	newQuat.z = p.w * q.z + p.z * q.w + p.x * q.y - p.y * q.x;
	return newQuat;
}

vec3 QuatMultiplyVec3(vec4 aQuat, vec3 aPos)
{
	// taken from GLM
	vec3 quatVector = vec3(aQuat.x, aQuat.y, aQuat.z);
	vec3 uv = cross(quatVector, aPos);
	vec3 uuv = cross(quatVector, uv);

	return aPos + ((uv * aQuat.w) + uuv) * 2.0;
}

vec3 ApplyBoneTransf(vec3 aPos, int anIndex)
{
	vec3 bonePos = Pos[anIndex];
	vec3 boneScale = Scale[anIndex];
	vec4 boneRot = Rotation[anIndex];

	return bonePos + QuatMultiplyVec3(boneRot, aPos) * boneScale;
}

vec4 ApplySkinning(vec3 aPos, ivec2 aBoneIndices, vec2 aBoneWeights)
{
	vec3 skinnedPos1 = ApplyBoneTransf(aPos, aBoneIndices.x);
	vec3 skinnedPos2 = ApplyBoneTransf(aPos, aBoneIndices.y);
	vec3 skinnedPos = skinnedPos1 * aBoneWeights.x + skinnedPos2 * aBoneWeights.y;

	return vec4(skinnedPos, 1.0);
}

void main() 
{
    gl_Position = MVP * ApplySkinning(position, boneIndices, boneWeights);
    normalOut = vec3(0);
    uvsOut = uvs;
}
