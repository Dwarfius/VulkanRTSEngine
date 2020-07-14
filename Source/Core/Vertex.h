#pragma once

#include <glm/gtx/hash.hpp>

struct Vertex
{
	constexpr static uint32_t Type = Utils::CRC32("Vertex");

	glm::vec3 myPos;
	glm::vec2 myUv;
	glm::vec3 myNormal;

	Vertex() = default;
	constexpr Vertex(glm::vec3 aPos, glm::vec3 aUv, glm::vec3 aNormal)
		: myPos(aPos)
		, myUv(aUv)
		, myNormal(aNormal)
	{
	}

	constexpr static uint8_t GetMemberCount()
	{
		return 3;
	}

	constexpr static size_t GetMemberOffset(uint8_t aMemberIndex)
	{
		switch (aMemberIndex)
		{
		case 0: return offsetof(Vertex, myPos);
		case 1: return offsetof(Vertex, myUv);
		case 2: return offsetof(Vertex, myNormal);
		default: ASSERT(false); return 0;
		}
	}

	constexpr static bool IsMemberIntegral(uint8_t aMemberIndex)
	{
		// oh static reflection, where are you >.>
		switch (aMemberIndex)
		{
		case 0: return false;
		case 1: return false;
		case 2: return false;
		default: ASSERT(false); return 0;
		}
	}
};
static_assert(std::is_trivially_destructible_v<Vertex>, "Not trivially destructible! Engine relies on cheap deallocations of vertices");

struct PosColorVertex
{
	constexpr static uint32_t Type = Utils::CRC32("PosColorVertex");

	glm::vec3 myPos;
	glm::vec3 myColor;

	PosColorVertex() = default;
	constexpr PosColorVertex(glm::vec3 aPos, glm::vec3 aColor)
		: myPos(aPos)
		, myColor(aColor)
	{
	}

	constexpr static uint8_t GetMemberCount()
	{
		return 2;
	}

	constexpr static size_t GetMemberOffset(uint8_t aMemberIndex)
	{
		switch (aMemberIndex)
		{
		case 0: return offsetof(PosColorVertex, myPos);
		case 1: return offsetof(PosColorVertex, myColor);
		default: ASSERT(false); return 0;
		}
	}

	constexpr static bool IsMemberIntegral(uint8_t aMemberIndex)
	{
		switch (aMemberIndex)
		{
		case 0: return false;
		case 1: return false;
		default: ASSERT(false); return false;
		}
	}
};
static_assert(std::is_trivially_destructible_v<PosColorVertex>, "Not trivially destructible! Engine relies on cheap deallocations of vertices");

struct ImGUIVertex
{
	constexpr static uint32_t Type = Utils::CRC32("ImGUIVertex");

	glm::vec2 myPos;
	glm::vec2 myUv;
	uint32_t myColor;

	ImGUIVertex() = default;
	constexpr ImGUIVertex(glm::vec2 aPos, glm::vec2 aUv, uint32_t aColor)
		: myPos(aPos)
		, myUv(aUv)
		, myColor(aColor)
	{
	}

	constexpr static uint8_t GetMemberCount()
	{
		return 3;
	}

	constexpr static size_t GetMemberOffset(uint8_t aMemberIndex)
	{
		switch (aMemberIndex)
		{
		case 0: return offsetof(ImGUIVertex, myPos);
		case 1: return offsetof(ImGUIVertex, myUv);
		case 2: return offsetof(ImGUIVertex, myColor);
		default: ASSERT(false); return 0;
		}
	}

	constexpr static bool IsMemberIntegral(uint8_t aMemberIndex)
	{
		switch (aMemberIndex)
		{
		case 0: return false;
		case 1: return false;
		case 2: return true;
		default: ASSERT(false); return false;
		}
	}
};
static_assert(std::is_trivially_destructible_v<ImGUIVertex>, "Not trivially destructible! Engine relies on cheap deallocations of vertices");

struct VertexHelper
{
	constexpr static size_t GetVertexSize(uint32_t aVertexType)
	{
		switch (aVertexType)
		{
		case Vertex::Type: return sizeof(Vertex);
		case PosColorVertex::Type: return sizeof(PosColorVertex);
		case ImGUIVertex::Type: return sizeof(ImGUIVertex);
		default: ASSERT(false); return 0;
		}
	}

	constexpr static uint8_t GetMemberCount(uint32_t aVertexType)
	{
		switch (aVertexType)
		{
		case Vertex::Type: return Vertex::GetMemberCount();
		case PosColorVertex::Type: return PosColorVertex::GetMemberCount();
		case ImGUIVertex::Type: return ImGUIVertex::GetMemberCount();
		default: ASSERT(false); return 0;
		}
	}

	constexpr static size_t GetMemberOffset(uint32_t aVertexType, uint8_t aMemberIndex)
	{
		switch (aVertexType)
		{
		case Vertex::Type: return Vertex::GetMemberOffset(aMemberIndex);
		case PosColorVertex::Type: return PosColorVertex::GetMemberOffset(aMemberIndex);
		case ImGUIVertex::Type: return ImGUIVertex::GetMemberOffset(aMemberIndex);
		default: ASSERT(false); return 0;
		}
	}

	constexpr static bool IsMemberIntegral(uint32_t aVertexType, uint8_t aMemberIndex)
	{
		switch (aVertexType)
		{
		case Vertex::Type: return Vertex::IsMemberIntegral(aMemberIndex);
		case PosColorVertex::Type: return PosColorVertex::IsMemberIntegral(aMemberIndex);
		case ImGUIVertex::Type: return ImGUIVertex::IsMemberIntegral(aMemberIndex);
		default: ASSERT(false); return 0;
		}
	}
};

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

	template<> struct hash<ImGUIVertex>
	{
		size_t operator()(const ImGUIVertex& vertex) const
		{
			return ((hash<glm::vec2>()(vertex.myPos) ^
				(hash<glm::vec2>()(vertex.myUv) << 1)) >> 1) ^
				(hash<uint32_t>()(vertex.myColor) << 1);
		}
	};
}

