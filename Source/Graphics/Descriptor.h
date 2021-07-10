#pragma once

#include <Core/DataEnum.h>

class Serializer;

// TODO: might be a good idea to use unpack-CRTP to write a fully static-compiled descriptors
// might help save on the memory access logic, and let compiler optimize the access better

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
	void SetUniformType(uint32_t aSlot, UniformType aType, uint32_t anArraySize);

	// Goes through the accumulated types and calculates the total size
	// Follows the std140 layout specification
	void RecomputeSize();

	// Returns how many uniforms does this descriptor have
	size_t GetUniformCount() const { return myEntries.size(); }
	// Returns the aligned size of the memory block that's 
	// needed to contain array of all types.
	// 16-byte alligned according to std140 layour specification
	size_t GetBlockSize() const { return myTotalSize; }
	// Returns the unaligned offset for a specific slot.
	// 4-byte alligned according to std140 layour specification
	char* GetPtr(uint32_t aSlot, size_t anArrayIndex, char* aBasePtr) const;
	// Returns the type of value stored at the specific slot
	UniformType GetType(uint32_t aSlot) const { return myEntries[aSlot].myUniformType; }

	size_t GetSlotSize(uint32_t aSlot) const;
	size_t GetArraySize(uint32_t aSlot) const { return myEntries[aSlot].myArraySize; }
	
	// Name of the adapter that is related to the Uniform Buffer Object
	const std::string& GetUniformAdapter() const { return myUniformAdapter; }

	void Serialize(Serializer& aSerializer);

private:
	void GetSizeAndAlignment(uint32_t aSlot, size_t& aSize, size_t& anAlignment) const;

	std::vector<Entry> myEntries;
	size_t myTotalSize = 0;
	std::string myUniformAdapter;
};