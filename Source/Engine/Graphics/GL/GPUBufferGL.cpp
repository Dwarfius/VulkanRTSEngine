#include "Precomp.h"
#include "GPUBufferGL.h"

#include <Graphics/Descriptor.h>
#include <Graphics/Graphics.h>

void GPUBufferGL::Cleanup()
{
	myState = State::PendingUnload;
	myGraphics->CleanUpGPUBuffer(this);
}

void GPUBufferGL::Bind(uint32_t aBindPoint)
{
	if (myType == Type::Uniform)
	{
		const Buffer& buffer = myBufferInfos.GetRead();
		glFlushMappedNamedBufferRange(myBufferGL, buffer.myOffest, myBufferSize);
		glBindBufferRange(GL_UNIFORM_BUFFER, aBindPoint, myBufferGL,
			buffer.myOffest, myBufferSize);
	}
	else
	{
		glFlushMappedNamedBufferRange(myBufferGL, 0, myBufferSize);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, aBindPoint, myBufferGL);
	}
}

void GPUBufferGL::OnCreate(Graphics& aGraphics)
{
	ASSERT_STR(!myBufferGL, "Double initialization of GL buffer!");
	glCreateBuffers(1, &myBufferGL);

	const size_t fullSize = myType == Type::Uniform ? myBufferSize * kMaxFrames : myBufferSize;
	constexpr uint32_t kCreateFlags = GL_MAP_WRITE_BIT
		| GL_MAP_PERSISTENT_BIT;
	glNamedBufferStorage(myBufferGL, fullSize, nullptr, kCreateFlags);

	constexpr uint32_t kMapFlags = GL_MAP_WRITE_BIT
		| GL_MAP_PERSISTENT_BIT
		| GL_MAP_FLUSH_EXPLICIT_BIT;
	myMappedBuffer = glMapNamedBufferRange(myBufferGL, 0, fullSize, kMapFlags);

	if (myType == Type::Uniform)
	{
		for (uint8_t i = 0; i < kMaxFrames; i++)
		{
			Buffer& buffer = myBufferInfos.GetWrite();
			buffer.myOffest = i * myBufferSize;
			myBufferInfos.Advance();
		}
	}
}

void GPUBufferGL::OnUnload(Graphics& aGraphics)
{
	ASSERT_STR(myBufferGL, "Unloading uninitialized GL buffer!");
	glDeleteBuffers(1, &myBufferGL);
}