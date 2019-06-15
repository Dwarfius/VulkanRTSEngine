#include "Precomp.h"
#include "PipelineGL.h"

#include <Graphics/Pipeline.h>
#include "ShaderGL.h"

PipelineGL::PipelineGL()
	: GPUResource()
	, myGLProgram(0)
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

	ASSERT_STR(mySamplerUniforms.size() < numeric_limits<GLint>::max(), "Sampler index doesn't fit!");
	// rebinding samplers to texture slots
	for(size_t i=0; i<mySamplerUniforms.size(); i++)
	{
		glUniform1i(mySamplerUniforms[i], static_cast<GLint>(i));
	}
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
#ifdef _DEBUG
		int length = 0;
		glGetProgramiv(myGLProgram, GL_INFO_LOG_LENGTH, &length);

		string errStr;
		errStr.resize(length);
		glGetProgramInfoLog(myGLProgram, length, &length, &errStr[0]);

		myErrorMsg = "Linking error: " + errStr;
#endif
	}
	else
	{
		// we've succesfully linked, time to resolve the UBOs
		size_t descCount = desc.myDescriptorCount;
		myBuffers.reserve(descCount);
		for (size_t i = 0; i < descCount; i++)
		{
			const Descriptor& descriptor = desc.myDescriptors[i];

			// TODO: get rid of this name hack, have a proper name string!
			const string& uboName = descriptor.GetUniformAdapter();
			uint32_t uboIndex = glGetUniformBlockIndex(myGLProgram, uboName.c_str());
			glUniformBlockBinding(myGLProgram, uboIndex, i);
			
			UniformBufferGL ubo;
			ubo.Create(descriptor.GetBlockSize());
			myBuffers.push_back(move(ubo));
		}

		// Because samplers can't be part of the uniform blocks, 
		// they have to stay separate, which means we have to find them
		// and track them
		int uniformCount = 0;
		glGetProgramiv(myGLProgram, GL_ACTIVE_UNIFORMS, &uniformCount);
		ASSERT(uniformCount >= 0);
		// We will need the name in order to find the location of the uniform
		int maxNameLength = 0;
		glGetProgramiv(myGLProgram, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxNameLength);
		string uniformName;
		uniformName.resize(maxNameLength + 1);
		for (int i = 0; i < uniformCount; i++)
		{
			int uniformSize = 0;
			uint32_t uniformType = 0;
			int uniformNameLength = 0;
			glGetActiveUniform(myGLProgram, i, maxNameLength, 
				&uniformNameLength, &uniformSize, &uniformType, &(uniformName[0]));
			uniformName[uniformNameLength] = 0; // chop off the rest

			switch (uniformType)
			{
			// We're only interested in the samplers, so only fetch them
			case GL_SAMPLER_2D:
			{
				int location = glGetUniformLocation(myGLProgram, uniformName.c_str());
				mySamplerUniforms.push_back(location);
				mySamplerTypes.push_back(uniformType);
			}
			break;
			}
		}
	}
	return linked;
}

void PipelineGL::Unload()
{
	ASSERT_STR(myGLProgram, "Empty pipeline detected!");
	glDeleteProgram(myGLProgram);
	myGLProgram = 0;
}