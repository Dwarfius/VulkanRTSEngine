#include "Precomp.h"
#include "PipelineGL.h"

#include "ShaderGL.h"
#include "UniformBufferGL.h"

PipelineGL::PipelineGL()
	: GPUResource()
	, myGLProgram(0)
	, myBuffer(nullptr)
{
}

PipelineGL::~PipelineGL()
{
	if (myGLProgram)
	{
		Unload();
	}
}

void PipelineGL::Bind()
{
	glUseProgram(myGLProgram);
}

void PipelineGL::Create(any aDescriptor)
{
	ASSERT_STR(!myGLProgram, "Pipeline already created!");
	myGLProgram = glCreateProgram();
}

bool PipelineGL::Upload(any aDescriptor)
{
	const Pipeline::UploadDescriptor& desc = 
		any_cast<const Pipeline::UploadDescriptor&>(aDescriptor);

	ASSERT_STR(myGLProgram, "Pipeline missing!");

	// Upload is just linking the dependencies on the GPU
	size_t shaderCount = desc.myShaderCount;
	for (size_t ind = 0; ind < shaderCount; ind++)
	{
		const ShaderGL* shader = static_cast<const ShaderGL*>(desc.myShaders[ind]);
		glAttachShader(myGLProgram, shader->GetShaderId());
	}
	glLinkProgram(myGLProgram);

	GLint isLinked = 0;
	glGetProgramiv(myGLProgram, GL_LINK_STATUS, &isLinked);

	bool linked = isLinked != GL_FALSE;
	if (!linked)
	{
		GLint length = 0;
		glGetProgramiv(myGLProgram, GL_INFO_LOG_LENGTH, &length);

		string errStr;
		errStr.resize(length);
		glGetProgramInfoLog(myGLProgram, length, &length, &errStr[0]);

		myErrorMsg = "Linking error: " + errStr;
	}
	else
	{
		if (desc.myDescriptor)
		{
			// we've succesfully linked, time to resolve the UBO
			// TODO: get rid of this name hack, have a proper name string!
			const Descriptor& descriptor = (*desc.myDescriptor);
			const string& uboName = descriptor.GetUniformAdapter();
			uint32_t uboIndex = glGetUniformBlockIndex(myGLProgram, uboName.c_str());
			glUniformBlockBinding(myGLProgram, uboIndex, 0); // point it at 0!
			myBuffer = new UniformBufferGL();
			myBuffer->Create(descriptor.GetBlockSize());
		}
	}
	return linked;
}

void PipelineGL::Unload()
{
	ASSERT_STR(myGLProgram, "Empty pipeline detected!");
	glDeleteProgram(myGLProgram);

	if (myBuffer)
	{
		delete myBuffer;
	}
}