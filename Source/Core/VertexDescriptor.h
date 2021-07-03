#pragma once

class Serializer;

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
	constexpr static const char* const kMemberTypeNames[]
	{
		"Bool",
		"U8",
		"S8",
		"U16",
		"S16",
		"U32",
		"S32",
		"F16",
		"F32",
		"F64"
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

		void Serialize(Serializer& aSerializer);
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

	void Serialize(Serializer& aSerializer);
};