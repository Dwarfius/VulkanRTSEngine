#pragma once

#include "GraphicsTypes.h"
#include <Core/Resources/Resource.h>

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
class Descriptor : public Resource
{
public:
	static constexpr StaticString kDir = Resource::AssetsFolder + "pipelineAdapters/";

public:
	Descriptor();
	Descriptor(Resource::Id anId, const std::string& aPath);

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

	// Name of the adapter that is related to the Uniform Buffer Object
	const std::string& GetUniformAdapter() const { return myUniformAdapter; }

	Type GetResType() const override final { return Type::Descriptor; };

private:
	void Serialize(Serializer& aSerializer) override final;

	std::vector<uint32_t> myOffsets;
	std::vector<UniformType> myTypes;
	size_t myTotalSize;
	std::string myUniformAdapter;
};