#pragma once

#include "Descriptor.h"

class UniformBuffer;

class UniformBlock
{
public:
	UniformBlock(UniformBuffer& aBuffer, const Descriptor& aDescriptor);
	~UniformBlock();

	template<class T>
	void SetUniform(uint32_t aSlot, size_t anArrayIndex, const T& aValue);

private:
	char* myData;
	UniformBuffer& myBuffer;
	const Descriptor& myDescriptor;
};

template<class T>
void UniformBlock::SetUniform(uint32_t aSlot, size_t anArrayIndex, const T& aValue)
{
	ASSERT_STR(aSlot < myDescriptor.GetUniformCount(), "Either invalid slot was provided, or block wasn't resolved!");
	ASSERT_STR(sizeof(T) <= myDescriptor.GetSlotSize(aSlot), "Size mismatch of passed data and slot requested!");
	ASSERT_STR(anArrayIndex < myDescriptor.GetArraySize(aSlot), "Inufficient buffer size, did you forget to call Resize?");
	T* slotPointer = reinterpret_cast<T*>(myData + myDescriptor.GetOffset(aSlot, anArrayIndex));
	*slotPointer = aValue;
}