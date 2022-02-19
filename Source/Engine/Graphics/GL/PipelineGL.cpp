#include "Precomp.h"
#include "PipelineGL.h"

#include <Graphics/Resources/Pipeline.h>
#include <Graphics/Graphics.h>
#include <Core/Resources/AssetTracker.h>
#include <Core/Profiler.h>

#include "ShaderGL.h"
#include "UniformBufferGL.h"


PipelineGL::PipelineGL()
	: myGLProgram(0)
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

	ASSERT_STR(mySamplerUniforms.size() < std::numeric_limits<GLint>::max(), "Sampler index doesn't fit!");
	// rebinding samplers to texture slots
	for(size_t i=0; i<mySamplerUniforms.size(); i++)
	{
		glUniform1i(mySamplerUniforms[i], static_cast<GLint>(i));
	}
}

void PipelineGL::OnCreate(Graphics& aGraphics)
{
	ASSERT_STR(!myGLProgram, "Pipeline already created!");
	myGLProgram = glCreateProgram();

	// request the creation of uniform buffers,
	// since they're not kept as part of myDependencies
	const Pipeline* pipeline = myResHandle.Get<const Pipeline>();
	size_t descCount = pipeline->GetDescriptorCount();
	myBuffers.reserve(descCount);
	myDescriptors.reserve(descCount);
	myAdapters.reserve(descCount);
	for(size_t i=0; i<descCount; i++)
	{
		const Descriptor& descriptor = pipeline->GetDescriptor(i);
		myDescriptors.push_back(descriptor);
		myAdapters.push_back(pipeline->GetAdapter(i));

		Handle<UniformBufferGL> ubo = new UniformBufferGL(descriptor);
		ubo->Create(aGraphics, nullptr);
		myBuffers.push_back(ubo);
	}

	size_t shaderCount = pipeline->GetShaderCount();
	for (size_t i = 0; i < shaderCount; i++)
	{
		const std::string& shaderName = pipeline->GetShader(i);
		Handle<Shader> shader = aGraphics.GetAssetTracker().GetOrCreate<Shader>(shaderName);
		myDependencies.push_back(aGraphics.GetOrCreate(shader));
	}
}

bool PipelineGL::OnUpload(Graphics& aGraphics)
{
	Profiler::ScopedMark uploadMark("PipelineGL::OnUpload");

	ASSERT_STR(myGLProgram, "Pipeline missing!");

	// Upload is just linking the dependencies on the GPU
	size_t shaderCount = myDependencies.size();
	for (size_t ind = 0; ind < shaderCount; ind++)
	{
		const ShaderGL* shader = myDependencies[ind].Get<const ShaderGL>();
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

		std::string errStr;
		errStr.resize(length);
		glGetProgramInfoLog(myGLProgram, length, &length, &errStr[0]);

		SetErrMsg("Linking error: " + errStr);
#endif
	}
	else
	{
		// we've succesfully linked, time to resolve the UBOs
		const Pipeline* pipeline = myResHandle.Get<const Pipeline>();
		const size_t descCount = pipeline->GetDescriptorCount();
		for (size_t i = 0; i < descCount; i++)
		{
			const Descriptor& descriptor = pipeline->GetDescriptor(i);

			// TODO: get rid of this name hack, have a proper name string!
			const std::string& uboName = descriptor.GetUniformAdapter();
			uint32_t uboIndex = glGetUniformBlockIndex(myGLProgram, uboName.c_str());
			ASSERT_STR(uboIndex != GL_INVALID_INDEX, "Got invalid index for %s", uboName.c_str());
			glUniformBlockBinding(myGLProgram, uboIndex, static_cast<GLint>(i));
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
		std::string uniformName;
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

void PipelineGL::OnUnload(Graphics& aGraphics)
{
	ASSERT_STR(myGLProgram, "Empty pipeline detected!");
	glDeleteProgram(myGLProgram);
	myGLProgram = 0;
}

bool PipelineGL::AreDependenciesValid() const
{
	if (!GPUResource::AreDependenciesValid())
	{
		return false;
	}
	for (Handle<UniformBufferGL> buffer : myBuffers)
	{
		if (buffer->GetState() != State::Valid)
		{
			return false;
		}
	}
	return true;
}