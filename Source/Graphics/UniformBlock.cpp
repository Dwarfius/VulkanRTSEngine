#include "Precomp.h"
#include "UniformBlock.h"

#include "Resources/UniformBuffer.h"

UniformBlock::UniformBlock(UniformBuffer& aBuffer, const Descriptor& aDescriptor)
	: myBuffer(aBuffer)
	, myDescriptor(aDescriptor)
{
	myData = myBuffer.Map();
}

UniformBlock::~UniformBlock()
{
	myBuffer.Unmap();
}