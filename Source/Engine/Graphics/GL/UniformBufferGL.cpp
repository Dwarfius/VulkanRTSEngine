#include "Precomp.h"
#include "UniformBufferGL.h"

#include <Graphics/Descriptor.h>

UniformBufferGL::UniformBufferGL(const Descriptor& aDescriptor)
	: myDescriptor(aDescriptor)
	, myBuffer(0)
{
	myUploadDesc.mySize = 0;
	myUploadDesc.myData = nullptr;
}

void UniformBufferGL::Bind(uint32_t aBindPoint)
{
	glBindBufferBase(GL_UNIFORM_BUFFER, aBindPoint, myBuffer);
}

bool UniformBufferGL::AreDependenciesValid() const
{
	if (!GPUResource::AreDependenciesValid())
	{
		return false;
	}
	return true;
}

void UniformBufferGL::OnCreate(Graphics& aGraphics)
{
	ASSERT_STR(!myBuffer, "Double initialization of uniform buffer!");

	glGenBuffers(1, &myBuffer);
}

bool UniformBufferGL::OnUpload(Graphics& aGraphics)
{
	// Despite the name, we don't upload, we just allocate, 
	// since we don't have anything to upload yet
	glBindBuffer(GL_UNIFORM_BUFFER, myBuffer);
	glBufferData(GL_UNIFORM_BUFFER, myDescriptor.GetBlockSize(), NULL, GL_DYNAMIC_DRAW);

	return true;
}

void UniformBufferGL::OnUnload(Graphics& aGraphics)
{
	ASSERT_STR(myBuffer, "Unloading uninitialized uniform buffer!");
	glDeleteBuffers(1, &myBuffer);
	myBuffer = 0;
}

void UniformBufferGL::UploadData(const UploadDescriptor& anUploadDesc)
{
	ASSERT_STR(myBuffer, "Uninitialized uniform buffer!");
	ASSERT_STR(anUploadDesc.mySize > 0 && anUploadDesc.myData, "Missing upload descriptor!");
	ASSERT_STR(anUploadDesc.mySize == myDescriptor.GetBlockSize(), "Missmatching upload desc!");
	myUploadDesc = anUploadDesc;

	glBindBuffer(GL_UNIFORM_BUFFER, myBuffer);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, myUploadDesc.mySize, myUploadDesc.myData);
}