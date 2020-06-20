#include "Precomp.h"
#include "UniformBlock.h"

UniformBlock::UniformBlock(const Descriptor& aDescriptor)
	: myData(nullptr)
	, myDescriptor(aDescriptor)
{
	// TODO: instead of allocating own storage, it should be requesting it
	// from the pool managed by Graphics.
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

UniformBlockPoolTraits::UniformBlockPoolTraits(Handle<Descriptor> aDescriptor)
	: myDescriptor(aDescriptor)
{
}

size_t UniformBlockPoolTraits::GetSize() const
{
	return myDescriptor->GetBlockSize();
}

