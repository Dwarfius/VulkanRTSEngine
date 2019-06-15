#pragma once

#include "GraphicsTypes.h"

#include <nlohmann/json_fwd.hpp>

// TODO: might be a good idea to use unpack-CRTP to write a fully static-compiled descriptors
// might help save on the memory access logic, and let compiler optimize the access better

// A class that describes how uniforms are stored for passing to the GPU.
// Follows the std140 layour.
// Use example:
// Descriptor d(1);
// d.SetUniformType(0, UniformType::Vec2);
// ...
// d.RecomputeSize();
// UniformBlock b(2, d);
class Descriptor
{
public:
	static bool FromJSON(const nlohmann::json& jsonHandle, Descriptor& aDesc);

public:
	Descriptor();
	Descriptor(const std::string& anAdapterName);

	// Marks the slot to be used storing a uniform of specific type
	void SetUniformType(uint32_t aSlot, UniformType aType);

	// Goes through the accumulated types and calculates the total size
	// Follows the std140 layout specification
	void RecomputeSize();

	// Returns how many uniforms does this descriptor have
	size_t GetUniformCount() const { return myTypes.size(); }
	// Returns the unaligned size of the memory block that's needed to contain all types.
	// 4-byte alligned according to std140 layour specification
	size_t GetBlockSize() const { return myTotalSize; }
	// Returns the unaligned offset for a specific slot.
	// 4-byte alligned according to std140 layour specification
	size_t GetOffset(uint32_t aSlot) const { return myOffsets[aSlot]; }
	// Returns the type of value stored at the specific slot
	UniformType GetType(uint32_t aSlot) const { return myTypes[aSlot]; }

	size_t GetSlotSize(uint32_t aSlot) const;

	// Sets the name of the uniform adapter that will be used to fetch
	// uniform data from game object to GPU
	const std::string& GetUniformAdapter() const { return myUniformAdapter; }

private:
	std::vector<uint32_t> myOffsets;
	std::vector<UniformType> myTypes;
	size_t myTotalSize;
	std::string myUniformAdapter;
};