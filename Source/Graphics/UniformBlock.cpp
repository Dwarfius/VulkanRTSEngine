#include "Precomp.h"
#include "UniformBlock.h"

#include "Resources/GPUBuffer.h"

UniformBlock::UniformBlock(GPUBuffer& aBuffer)
	: myBuffer(aBuffer)
{
	myData = myBuffer.Map();
}

UniformBlock::~UniformBlock()
{
	myBuffer.Unmap();
}