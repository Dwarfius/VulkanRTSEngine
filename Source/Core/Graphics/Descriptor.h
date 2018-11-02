#pragma once

#include "GraphicsTypes.h"

// TODO: might be a good idea to use unpack-CRTP to write a fully static-compiled descriptors
// might help save on the memory access logic, and let compiler optimize the access better

// A class that maps that holds the definition of memory accessses for GPU.
// Use example:
// Descriptor d(1);
// d.SetUniformType(0, UniformType::Vec2);
// ...
// d.RecomputeSize();
// UniformBlock b(2, d);
class Descriptor
{
public:
	Descriptor();

	// Marks the slot to be used storing a uniform of specific type
	void SetUniformType(uint32_t aSlot, UniformType aType);

	// Goes through the accumulated types and calculates the total size
	void RecomputeSize();

	// Returns how many uniforms does this descriptor have
	size_t GetUniformCount() const { return myTypes.size(); }
	// Returns the unaligned size of the memory block that's needed to contain all types.
	size_t GetBlockSize() const { return myTotalSize; }
	// Returns the unaligned offset for a specific slot.
	size_t GetOffset(uint32_t aSlot) const { return myOffsets[aSlot]; }
	// Returns the type of value stored at the specific slot
	UniformType GetType(uint32_t aSlot) const { return myTypes[aSlot]; }

private:
	vector<uint32_t> myOffsets;
	vector<UniformType> myTypes;
	size_t myTotalSize;
};