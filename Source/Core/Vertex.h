#pragma once

#include <glm/gtx/hash.hpp>

struct Vertex
{
	constexpr static int Type = 0;

	glm::vec3 myPos;
	glm::vec2 myUv;
	glm::vec3 myNormal;

	bool operator==(const Vertex& other) const
	{
		return myPos == other.myPos && myUv == other.myUv && myNormal == other.myNormal;
	}
};
// Vertex and PosColorVertex are intended to be lightweight, cheap structs
static_assert(is_trivially_destructible_v<Vertex>, "Not trivially destructible!");

struct PosColorVertex
{
	constexpr static int Type = 1;

	glm::vec3 myPos;
	glm::vec3 myColor;

	bool operator==(const PosColorVertex& other) const
	{
		return myPos == other.myPos && myColor == other.myColor;
	}
};
static_assert(is_trivially_destructible_v<PosColorVertex>, "Not trivially destructible!");

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