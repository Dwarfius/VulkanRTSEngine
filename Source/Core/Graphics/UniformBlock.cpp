#include "Precomp.h"
#include "UniformBlock.h"

#include "Descriptor.h"

UniformBlock::UniformBlock(const Descriptor& aDescriptor)
	: myData(nullptr)
	, myDescriptor(aDescriptor)
{
	// TODO: instead of allocating own storage, it should be requesting it
	// from the pool managed by Graphics.
	// TODO: there might be 4byte alignment requirements for Vulkan
	myData = new char[myDescriptor.GetBlockSize()];
}

UniformBlock::~UniformBlock()
{
	delete[] myData;
}

size_t UniformBlock::GetSize() const
{
	return myDescriptor.GetBlockSize();
}