#include "Precomp.h"
#include "UniformBufferGL.h"

UniformBufferGL::UniformBufferGL(size_t aBufferSize)
	: myBuffer(0)
	, myBufferSize(aBufferSize)
{
	myUploadDesc.mySize = 0;
	myUploadDesc.myData = nullptr;
}

void UniformBufferGL::Bind(uint32_t aBindPoint)
{
	glBindBufferBase(GL_UNIFORM_BUFFER, aBindPoint, myBuffer);
}

void UniformBufferGL::OnCreate(Graphics& aGraphics)
{
	ASSERT_STR(!myBuffer, "Double initialization of uniform buffer!");

	glGenBuffers(1, &myBuffer);

	glBindBuffer(GL_UNIFORM_BUFFER, myBuffer);
	glBufferData(GL_UNIFORM_BUFFER, myBufferSize, NULL, GL_DYNAMIC_DRAW);
}

bool UniformBufferGL::OnUpload(Graphics& aGraphics)
{
	if (myUploadDesc.mySize > 0)
	{
		UploadData(myUploadDesc);
	}
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
	myUploadDesc = anUploadDesc;

	glBindBuffer(GL_UNIFORM_BUFFER, myBuffer);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, myUploadDesc.mySize, myUploadDesc.myData);
}