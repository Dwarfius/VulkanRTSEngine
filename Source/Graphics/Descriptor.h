#pragma once

#include <Core/DataEnum.h>

class Serializer;

// A class that describes how uniforms are stored for passing to the GPU.
// Follows the std140 layout. Only supports AoS layout.
// Use example:
// Descriptor d();
// d.SetUniformType(0, UniformType::Vec2);
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
		Vec3,
		Vec4,
		Mat4
	);

private:
	struct Entry
	{
		size_t myByteOffset = 0;
		uint32_t myArraySize = 1;
		UniformType myUniformType = UniformType::Int;
	};

public:
	// Marks the slot to be used storing a uniform of specific type
	constexpr void SetUniformType(uint32_t aSlot, UniformType aType, uint32_t anArraySize);
	constexpr void AddUniformType(UniformType aType = UniformType::Int, uint32_t anArraySize = 1);
	constexpr void RemoveUniformType(uint32_t aSlot);

	// Goes through the accumulated types and calculates the total size
	// Follows the std140 layout specification
	constexpr void RecomputeSize();

	// Returns how many uniforms does this descriptor have
	constexpr size_t GetUniformCount() const { return myEntries.size(); }
	// Returns the aligned size of the memory block that's 
	// needed to contain array of all types.
	// 16-byte alligned according to std140 layour specification
	constexpr size_t GetBlockSize() const { return myTotalSize; }
	// Returns the unaligned offset for a specific slot.
	// 4-byte alligned according to std140 layour specification
	constexpr char* GetPtr(uint32_t aSlot, size_t anArrayIndex, char* aBasePtr) const;
	// Returns the type of value stored at the specific slot
	constexpr UniformType GetType(uint32_t aSlot) const { return myEntries[aSlot].myUniformType; }

	constexpr size_t GetSlotSize(uint32_t aSlot) const;
	constexpr uint32_t GetArraySize(uint32_t aSlot) const { return myEntries[aSlot].myArraySize; }
	
	// Name of the adapter that is related to the Uniform Buffer Object
	std::string_view GetUniformAdapter() const { return myUniformAdapter; }
	void SetUniformAdapter(const std::string& anAdapter) { myUniformAdapter = anAdapter; }

	void Serialize(Serializer& aSerializer);

private:
	constexpr void GetSizeAndAlignment(uint32_t aSlot, size_t& aSize, size_t& anAlignment) const;

	std::vector<Entry> myEntries;
	size_t myTotalSize = 0;
	std::string myUniformAdapter;
};

constexpr void Descriptor::SetUniformType(uint32_t aSlot, UniformType aType, uint32_t anArraySize)
{
	if (aSlot >= myEntries.size())
	{
		myEntries.resize(aSlot + 1llu);
	}
	myEntries[aSlot] = { 0, anArraySize, aType };
}

constexpr void Descriptor::AddUniformType(UniformType aType /* = UniformType::Int*/, uint32_t anArraySize /* = 1 */)
{
	myEntries.push_back({ 0, anArraySize, aType });
}

constexpr void Descriptor::RemoveUniformType(uint32_t aSlot)
{
	myEntries.erase(myEntries.begin() + aSlot);
}

constexpr void Descriptor::RecomputeSize()
{
	const size_t size = myEntries.size();

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

constexpr char* Descriptor::GetPtr(uint32_t aSlot, size_t anArrayIndex, char* aBasePtr) const
{
	const Entry& entry = myEntries[aSlot];
	size_t slotSize = 0;
	size_t ignored;
	GetSizeAndAlignment(aSlot, slotSize, ignored);
	return aBasePtr + entry.myByteOffset + anArrayIndex * slotSize;
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
		anAlignment = sizeof(int);
		aSize = sizeof(int);
		break;
	case UniformType::Vec2:
		anAlignment = sizeof(glm::vec2);
		aSize = sizeof(glm::vec2);
		break;
	case UniformType::Vec3:
	{
		// std140 - https://www.khronos.org/registry/OpenGL/specs/gl/glspec45.core.pdf#page=159
		// If the member is a three-component vector with components consuming N
		// basic machine units, the base alignment is 4N
		// Despite that, glsl compiler can merge the next single unit
		// to this 3, to avoid wasting MUs.
		bool canMerge = entry.myArraySize == 1
			&& aSlot + 1llu < myEntries.size()
			&& (myEntries[aSlot + 1llu].myUniformType == UniformType::Float
				|| myEntries[aSlot + 1llu].myUniformType == UniformType::Int)
			&& myEntries[aSlot + 1llu].myArraySize == 1;
		if (canMerge)
		{
			anAlignment = sizeof(glm::vec4);
			aSize = sizeof(glm::vec3);
		}
		else
		{
			anAlignment = sizeof(glm::vec4);
			aSize = sizeof(glm::vec4);
		}
		break;
	}
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