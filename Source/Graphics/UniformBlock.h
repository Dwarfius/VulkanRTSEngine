#pragma once

#include "Descriptor.h"

// A class representing a single block of uniforms for submitting to the GPU.
// Use:
// UniformBlock block(descriptor);
// block.SetUniform<glm::vec3>(0, glm::vec3(0.f));
class UniformBlock
{
public:
	UniformBlock(const Descriptor& aDescriptor);
	~UniformBlock();

	template<class T>
	void SetUniform(uint32_t aSlot, size_t anArrayIndex, const T& aValue);

	const char* GetData() const { return myData; }

	// 4-byte alligned according to std140 layour specification
	size_t GetSize() const;

private:
	char* myData;
	Descriptor myDescriptor;
};

template<class T>
void UniformBlock::SetUniform(uint32_t aSlot, size_t anArrayIndex, const T& aValue)
{
	ASSERT_STR(aSlot < myDescriptor.GetUniformCount(), "Either invalid slot was provided, or block wasn't resolved!");
	ASSERT_STR(sizeof(T) <= myDescriptor.GetSlotSize(aSlot), "Size mismatch of passed data and slot requested!");
	ASSERT_STR(anArrayIndex < myDescriptor.GetArraySize(aSlot), "Inufficient buffer size, did you forget to call Resize?");
	T* slotPointer = reinterpret_cast<T*>(myDescriptor.GetPtr(aSlot, anArrayIndex, myData));
	*slotPointer = aValue;
}