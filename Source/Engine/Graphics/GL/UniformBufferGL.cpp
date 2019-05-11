#include "Precomp.h"
#include "UniformBufferGL.h"

UniformBufferGL::UniformBufferGL()
	: myBuffer(0)
{
}

UniformBufferGL::~UniformBufferGL()
{
	if (myBuffer)
	{
		Unload();
	}
}

UniformBufferGL::UniformBufferGL(UniformBufferGL&& anOther)
{
	myBuffer = exchange(anOther.myBuffer, 0);
}

void UniformBufferGL::Bind(uint32_t aBindPoint)
{
	glBindBufferBase(GL_UNIFORM_BUFFER, aBindPoint, myBuffer);
}

void UniformBufferGL::Create(any aDescriptor)
{
	ASSERT_STR(!myBuffer, "Double initialization of uniform buffer!");

	size_t size = any_cast<size_t>(aDescriptor);
	glGenBuffers(1, &myBuffer);

	glBindBuffer(GL_UNIFORM_BUFFER, myBuffer);
	glBufferData(GL_UNIFORM_BUFFER, size, NULL, GL_DYNAMIC_DRAW);
}

bool UniformBufferGL::Upload(any aDescriptor)
{
	ASSERT_STR(myBuffer, "Uninitialized uniform buffer!");

	const UploadDescriptor& descriptor = any_cast<const UploadDescriptor&>(aDescriptor);
	
	glBindBuffer(GL_UNIFORM_BUFFER, myBuffer);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, descriptor.mySize, descriptor.myData);
	return true;
}

void UniformBufferGL::Unload()
{
	ASSERT_STR(myBuffer, "Unloading uninitialized uniform buffer!");
	glDeleteBuffers(1, &myBuffer);
	myBuffer = 0;
}