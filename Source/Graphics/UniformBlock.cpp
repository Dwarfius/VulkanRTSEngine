#include "Precomp.h"
#include "UniformBlock.h"

#include "Resources/UniformBuffer.h"

UniformBlock::UniformBlock(UniformBuffer& aBuffer)
	: myBuffer(aBuffer)
{
	myData = myBuffer.Map();
}

UniformBlock::~UniformBlock()
{
	myBuffer.Unmap();
}