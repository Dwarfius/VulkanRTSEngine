#include "Precomp.h"
#include "Descriptor.h"

#include <Core/Resources/Serializer.h>

Descriptor::Descriptor(std::initializer_list<InitArgs> aUniformList)
{
	for (InitArgs uniform : aUniformList)
	{
		AddUniformType(uniform.myType, uniform.myArraySize);
	}
	RecomputeSize();
}

void Descriptor::SetUniformType(uint32_t aSlot, UniformType aType, uint32_t anArraySize)
{
	if (aSlot >= myEntries.size())
	{
		myEntries.resize(aSlot + 1llu);
	}
	myEntries[aSlot] = { 0, anArraySize, aType };
}

void Descriptor::AddUniformType(UniformType aType /* = UniformType::Int*/, uint32_t anArraySize /* = 1 */)
{
	myEntries.push_back({ 0, anArraySize, aType });
}

void Descriptor::RemoveUniformType(uint32_t aSlot)
{
	myEntries.erase(myEntries.begin() + aSlot);
}

void Descriptor::RecomputeSize()
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

char* Descriptor::GetPtr(uint32_t aSlot, size_t anArrayIndex, char* aBasePtr) const
{
	const Entry& entry = myEntries[aSlot];
	size_t slotSize = 0;
	size_t ignored;
	GetSizeAndAlignment(aSlot, slotSize, ignored);
	return aBasePtr + entry.myByteOffset + anArrayIndex * slotSize;
}

size_t Descriptor::GetSlotSize(uint32_t aSlot) const
{
	size_t slotSize = 0;
	size_t ignored;
	GetSizeAndAlignment(aSlot, slotSize, ignored);
	return slotSize * myEntries[aSlot].myArraySize;
}

void Descriptor::GetSizeAndAlignment(uint32_t aSlot, size_t& aSize, size_t& anAlignment) const
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