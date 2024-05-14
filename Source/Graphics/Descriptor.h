#pragma once

#include <Core/DataEnum.h>
#include <Core/StaticVector.h>

class Serializer;

// A class that describes how uniforms are stored for passing to the GPU.
// Follows the std140 layout. Only supports AoS layout. 
// Has capacity for 16 entries!
// Use example:
// Descriptor d();
// d.AddUniformType(UniformType::Vec2, 20);
// ...
// d.RecomputeSize();
// UniformBlock b(2, d);
class Descriptor
{
public:
	DATA_ENUM(UniformType, char,
		Int,
		Float,
		Vec2,
		Vec4,
		Mat4
	);

	struct InitArgs
	{
		UniformType myType;
		uint32_t myArraySize = 1;
	};

private:
	struct Entry
	{
		size_t myByteOffset = 0;
		uint32_t myArraySize = 1;
		UniformType myUniformType = UniformType::Int;
	};

public:
	constexpr Descriptor() = default;
	constexpr Descriptor(std::initializer_list<InitArgs> aUniformsList);

	constexpr void AddUniformType(UniformType aType = UniformType::Int, uint32_t anArraySize = 1);

	// Goes through the accumulated types and calculates the total size
	// Follows the std140 layout specification
	constexpr void RecomputeSize();

	// Returns how many uniforms does this descriptor have
	constexpr size_t GetUniformCount() const { return myEntries.GetSize(); }
	// Returns the aligned size of the memory block that's 
	// needed to contain array of all types.
	// 16-byte alligned according to std140 layour specification
	constexpr size_t GetBlockSize() const { return myTotalSize; }
	// Returns the unaligned offset for a specific slot.
	// 4-byte alligned according to std140 layour specification
	constexpr size_t GetOffset(uint32_t aSlot, size_t anArrayIndex) const;
	// Returns the type of value stored at the specific slot
	constexpr UniformType GetType(uint32_t aSlot) const { return myEntries[aSlot].myUniformType; }

	constexpr size_t GetSlotSize(uint32_t aSlot) const;
	constexpr uint32_t GetArraySize(uint32_t aSlot) const { return myEntries[aSlot].myArraySize; }
	
private:
	constexpr void GetSizeAndAlignment(uint32_t aSlot, size_t& aSize, size_t& anAlignment) const;

	StaticVector<Entry, 16> myEntries;
	size_t myTotalSize = 0;
};

constexpr Descriptor::Descriptor(std::initializer_list<InitArgs> aUniformList)
{
	for (InitArgs uniform : aUniformList)
	{
		AddUniformType(uniform.myType, uniform.myArraySize);
	}
	RecomputeSize();
}

constexpr void Descriptor::AddUniformType(UniformType aType /* = UniformType::Int*/, uint32_t anArraySize /* = 1 */)
{
	myEntries.PushBack({ 0, anArraySize, aType });
}

constexpr void Descriptor::RecomputeSize()
{
	const size_t size = myEntries.GetSize();

	myTotalSize = 0;
	for (uint32_t i = 0; i < size; i++)
	{
		size_t basicAlignment = 0;
		size_t elemSize = 0;
		GetSizeAndAlignment(i, elemSize, basicAlignment);

		// myTotalSize has the aligned offset from the previous iter
		// plus the size of the new element.
		// calculate the adjusted size (with padding), so that we can
		// retain alignment required for new element
		size_t remainder = myTotalSize % basicAlignment;
		if (remainder)
		{
			myTotalSize += basicAlignment - remainder;
		}

		Entry& entry = myEntries[i];
		entry.myByteOffset = myTotalSize;
		myTotalSize += elemSize * entry.myArraySize;
	}

	// since it's a struct, per std140 rules it has to be 16-aligned
	constexpr size_t kStructAlignment = 16;
	size_t remainder = myTotalSize % kStructAlignment;
	if (remainder)
	{
		myTotalSize += kStructAlignment - remainder;
	}
}

constexpr size_t Descriptor::GetOffset(uint32_t aSlot, size_t anArrayIndex) const
{
	const Entry& entry = myEntries[aSlot];
	size_t slotSize = 0;
	size_t ignored;
	GetSizeAndAlignment(aSlot, slotSize, ignored);
	return entry.myByteOffset + anArrayIndex * slotSize;
}

constexpr size_t Descriptor::GetSlotSize(uint32_t aSlot) const
{
	size_t slotSize = 0;
	size_t ignored;
	GetSizeAndAlignment(aSlot, slotSize, ignored);
	return slotSize * myEntries[aSlot].myArraySize;
}

constexpr void Descriptor::GetSizeAndAlignment(uint32_t aSlot, size_t& aSize, size_t& anAlignment) const
{
	const Entry& entry = myEntries[aSlot];
	switch (entry.myUniformType)
	{
	case UniformType::Float:
	case UniformType::Int:
		// According to spec, arrays of scalars and vectors have
		// multiple of vec4 size&alignment
		anAlignment = entry.myArraySize == 1 ? sizeof(int) : sizeof(glm::vec4);
		aSize = entry.myArraySize == 1 ? sizeof(int) : sizeof(glm::vec4);
		break;
	case UniformType::Vec2:
		anAlignment = entry.myArraySize == 1 ? sizeof(glm::vec2) : sizeof(glm::vec4);
		aSize = entry.myArraySize == 1 ? sizeof(glm::vec2) : sizeof(glm::vec4);
		break;
	case UniformType::Vec4:
		anAlignment = sizeof(glm::vec4);
		aSize = sizeof(glm::vec4);
		break;
	case UniformType::Mat4:
		anAlignment = sizeof(glm::vec4);
		aSize = sizeof(glm::mat4);
		break;
	default:
		ASSERT_STR(false, "Unrecognized Uniform Type found!");
	}
}