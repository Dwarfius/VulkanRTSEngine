#pragma once

#include "DataEnum.h"
#include "StaticVector.h"

class Serializer;

struct VertexDescriptor
{
	DATA_ENUM(MemberType, uint8_t, 
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
	);

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
	static constexpr VertexDescriptor GetEmpty() { return { 0, {} }; }

	uint8_t mySize;
	StaticVector<MemberDescriptor, kMaxMembers> myMembers;

	// TODO: replace with c++20 operator<=>!
	constexpr bool operator==(const VertexDescriptor& aOther) const
	{
		auto CheckMembers = [](const VertexDescriptor& aLeft, const VertexDescriptor& aRight)
		{
			for (uint8_t index = 0; index < aLeft.myMembers.GetSize(); index++)
			{
				if (aLeft.myMembers[index] != aRight.myMembers[index])
				{
					return false;
				}
			}
			return true;
		};
		return mySize == aOther.mySize
			&& myMembers.GetSize() == aOther.myMembers.GetSize()
			&& CheckMembers(*this, aOther);
	}

	constexpr bool operator!=(const VertexDescriptor& aOther) const
	{
		return !(*this == aOther);
	}

	void Serialize(Serializer& aSerializer);
};