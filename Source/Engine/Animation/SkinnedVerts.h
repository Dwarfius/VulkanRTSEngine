#pragma once

struct SkinnedVertex
{
	glm::vec3 myPos;
	glm::vec3 myNormal;
	glm::vec2 myUv;
	uint32_t myBoneIndices[4];
	glm::vec4 myBoneWeights;

	static constexpr VertexDescriptor GetDescriptor()
	{
		using ThisType = SkinnedVertex; // for copy-paste convenience
		return {
			sizeof(ThisType),
			5,
			{
				{ VertexDescriptor::MemberType::F32, 3, offsetof(ThisType, myPos) },
				{ VertexDescriptor::MemberType::F32, 3, offsetof(ThisType, myNormal) },
				{ VertexDescriptor::MemberType::F32, 2, offsetof(ThisType, myUv) },
				{ VertexDescriptor::MemberType::U32, 4, offsetof(ThisType, myBoneIndices) },
				{ VertexDescriptor::MemberType::F32, 4, offsetof(ThisType, myBoneWeights) },
			}
		};
	}
};

namespace std
{
	template<> struct hash<SkinnedVertex>
	{
		std::size_t operator()(const SkinnedVertex& v) const
		{
			return hash<glm::vec3>()(v.myPos) ^
				((hash<glm::vec3>()(v.myNormal) >> 1) << 1) ^
				((hash<glm::vec2>()(v.myUv) << 1) >> 1) ^
				(hash<uint16_t>()(v.myBoneIndices[0]) << 2) ^
				(hash<uint16_t>()(v.myBoneIndices[1]) >> 2) ^
				(hash<uint16_t>()(v.myBoneIndices[2]) << 4) ^
				(hash<uint16_t>()(v.myBoneIndices[3]) >> 4) ^
				(hash<glm::vec4>()(v.myBoneWeights) >> 2);
		}
	};
}