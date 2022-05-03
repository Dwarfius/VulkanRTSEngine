#include "Precomp.h"
#include "UniformBufferGL.h"

#include <Graphics/Descriptor.h>
#include <Graphics/Graphics.h>

void UniformBufferGL::Cleanup()
{
	myGraphics->CleanUpUBO(this);
}

void UniformBufferGL::Bind(uint32_t aBindPoint)
{
	const Buffer& buffer = myBufferInfos.GetRead();
	glFlushMappedNamedBufferRange(myUniformBuffer, buffer.myOffest, myBufferSize);
	glBindBufferRange(GL_UNIFORM_BUFFER, aBindPoint, myUniformBuffer, 
		buffer.myOffest, myBufferSize);
}

void UniformBufferGL::OnCreate(Graphics& aGraphics)
{
	ASSERT_STR(!myUniformBuffer, "Double initialization of uniform buffer!");
	glCreateBuffers(1, &myUniformBuffer);

	constexpr uint32_t kCreateFlags = GL_MAP_WRITE_BIT
		| GL_MAP_PERSISTENT_BIT;
	glNamedBufferStorage(myUniformBuffer, myBufferSize * kMaxFrames, nullptr, kCreateFlags);

	constexpr uint32_t kMapFlags = GL_MAP_WRITE_BIT
		| GL_MAP_PERSISTENT_BIT
		| GL_MAP_FLUSH_EXPLICIT_BIT;
	myMappedBuffer = glMapNamedBufferRange(myUniformBuffer, 0, myBufferSize * kMaxFrames, kMapFlags);

	for (uint8_t i = 0; i < kMaxFrames; i++)
	{
		Buffer& buffer = myBufferInfos.GetWrite();
		buffer.myOffest = i * myBufferSize;
		myBufferInfos.Advance();
	}
}

void UniformBufferGL::OnUnload(Graphics& aGraphics)
{
	ASSERT_STR(myUniformBuffer, "Unloading uninitialized uniform buffer!");
	glDeleteBuffers(1, &myUniformBuffer);
}