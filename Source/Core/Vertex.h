#pragma once

#include <glm/gtx/hash.hpp>
#include "VertexDescriptor.h"

struct Vertex
{
	glm::vec3 myPos;
	glm::vec2 myUv;
	glm::vec3 myNormal;

	Vertex() = default;
	constexpr Vertex(glm::vec3 aPos, glm::vec2 aUv, glm::vec3 aNormal)
		: myPos(aPos)
		, myUv(aUv)
		, myNormal(aNormal)
	{
	}

	static constexpr VertexDescriptor GetDescriptor()
	{
		using ThisType = Vertex; // for copy-paste convenience
		return {
			sizeof(ThisType),
			3,
			{  
				{ VertexDescriptor::MemberType::F32, 3, offsetof(ThisType, myPos) },
				{ VertexDescriptor::MemberType::F32, 2, offsetof(ThisType, myUv) },
				{ VertexDescriptor::MemberType::F32, 3, offsetof(ThisType, myNormal) }
			}
		};
	}
};
static_assert(std::is_trivially_destructible_v<Vertex>, "Not trivially destructible! Engine relies on cheap deallocations of vertices");

struct PosColorVertex
{
	glm::vec3 myPos;
	glm::vec3 myColor;

	PosColorVertex() = default;
	constexpr PosColorVertex(glm::vec3 aPos, glm::vec3 aColor)
		: myPos(aPos)
		, myColor(aColor)
	{
	}

	static constexpr VertexDescriptor GetDescriptor()
	{
		using ThisType = PosColorVertex; // for copy-paste convenience
		return {
			sizeof(ThisType),
			2,
			{
				{ VertexDescriptor::MemberType::F32, 3, offsetof(ThisType, myPos) },
				{ VertexDescriptor::MemberType::F32, 3, offsetof(ThisType, myColor) }
			}
		};
	}
};
static_assert(std::is_trivially_destructible_v<PosColorVertex>, "Not trivially destructible! Engine relies on cheap deallocations of vertices");

namespace std
{
	template<> struct hash<Vertex>
	{
		size_t operator()(const Vertex& vertex) const
		{
			return ((hash<glm::vec3>()(vertex.myPos) ^
				(hash<glm::vec3>()(vertex.myNormal) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.myUv) << 1);
		}
	};

	template<> struct hash<PosColorVertex>
	{
		size_t operator()(const PosColorVertex& vertex) const
		{
			return hash<glm::vec3>()(vertex.myPos) ^ 
				(hash<glm::vec3>()(vertex.myColor) << 1);
		}
	};
}

