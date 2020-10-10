#pragma once

#include <glm/gtx/hash.hpp>

struct VertexDescriptor
{
	enum class MemberType : uint8_t
	{
		Bool,
		U8,
		S8,
		U16,
		S16,
		U32,
		S32,
		F16,
		F32,
		F64
	};

	struct MemberDescriptor
	{
		MemberType myType;
		uint8_t myElemCount;
		uint8_t myOffset;

		// TODO: replace with c++20 operator<=>!
		constexpr bool operator==(const MemberDescriptor& aOther) const
		{
			return myType == aOther.myType
				&& myElemCount == aOther.myElemCount
				&& myOffset == aOther.myOffset;
		}

		constexpr bool operator!=(const MemberDescriptor& aOther) const
		{
			return !(*this == aOther);
		}
	};

	static constexpr uint8_t kMaxMembers = 16;
	static constexpr VertexDescriptor GetEmpty() { return { 0, 0, {} }; }

	uint8_t mySize;
	uint8_t myMemberCount;
	MemberDescriptor myMembers[kMaxMembers];

	// TODO: replace with c++20 operator<=>!
	constexpr bool operator==(const VertexDescriptor& aOther) const
	{
		auto CheckMembers = [](const VertexDescriptor& aLeft, const VertexDescriptor& aRight)
		{
			for (uint8_t index = 0; index < aLeft.myMemberCount; index++)
			{
				if (aLeft.myMembers[index] != aRight.myMembers[index])
				{
					return false;
				}
			}
			return true;
		};
		return mySize == aOther.mySize
			&& myMemberCount == aOther.myMemberCount
			&& CheckMembers(*this, aOther);
	}

	constexpr bool operator!=(const VertexDescriptor& aOther) const
	{
		return !(*this == aOther);
	}
};

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

	static constexpr VertexDescriptor GetDescriptor()
	{
		using ThisType = ImGUIVertex; // for copy-paste convenience
		return {
			sizeof(ThisType),
			3,
			{
				{ VertexDescriptor::MemberType::F32, 3, offsetof(ThisType, myPos) },
				{ VertexDescriptor::MemberType::F32, 3, offsetof(ThisType, myUv) },
				{ VertexDescriptor::MemberType::U32, 1, offsetof(ThisType, myColor) }
			}
		};
	}
};
static_assert(std::is_trivially_destructible_v<ImGUIVertex>, "Not trivially destructible! Engine relies on cheap deallocations of vertices");

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

