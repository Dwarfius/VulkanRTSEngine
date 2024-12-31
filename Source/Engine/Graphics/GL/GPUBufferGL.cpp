#include "Precomp.h"
#include "GPUBufferGL.h"

#include <Graphics/Descriptor.h>
#include <Graphics/Graphics.h>

void GPUBufferGL::Cleanup()
{
	myState = State::PendingUnload;
	myGraphics->CleanUpGPUBuffer(this);
}

void GPUBufferGL::Bind(uint32_t aBindPoint, uint32_t aBindPointType)
{
	CheckOverlap();
	const size_t offset = myReadHead * myBufferSize;
	glFlushMappedNamedBufferRange(myBufferGL, offset, myBufferSize);
	glBindBufferRange(aBindPointType, aBindPoint, myBufferGL, offset, myBufferSize);
}

void GPUBufferGL::OnCreate(Graphics& aGraphics)
{
	ASSERT_STR(!myBufferGL, "Double initialization of GL buffer!");
	glCreateBuffers(1, &myBufferGL);

	const size_t fullSize = myBufferSize * myFrameCount;
	constexpr uint32_t kCreateFlags = GL_MAP_WRITE_BIT
		| GL_MAP_PERSISTENT_BIT;
	glNamedBufferStorage(myBufferGL, fullSize, nullptr, kCreateFlags);

	constexpr uint32_t kMapFlags = GL_MAP_WRITE_BIT
		| GL_MAP_PERSISTENT_BIT
		| GL_MAP_FLUSH_EXPLICIT_BIT;
	myMappedBuffer = glMapNamedBufferRange(myBufferGL, 0, fullSize, kMapFlags);
}

void GPUBufferGL::OnUnload(Graphics& aGraphics)
{
	ASSERT_STR(myBufferGL, "Unloading uninitialized GL buffer!");
	glDeleteBuffers(1, &myBufferGL);
}