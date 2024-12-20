#include "Precomp.h"
#include "PipelineGL.h"

#include <Graphics/Resources/Pipeline.h>
#include <Graphics/Graphics.h>
#include <Graphics/Descriptor.h>
#include <Core/Resources/AssetTracker.h>
#include <Core/Profiler.h>

#include "ShaderGL.h"
#include "GPUBufferGL.h"

PipelineGL::PipelineGL()
	: myGLProgram(0)
{
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

void PipelineGL::Cleanup()
{
	GPUPipeline::Cleanup();
	// We drop shaders earlier (before OnUnload) so that
	// their removal can be scheduled earlier
	for (Handle<ShaderGL>& shaderGL : myShaders)
	{
		OnShaderRemove(*shaderGL.Get());
	}
	myShaders.clear();
}

void PipelineGL::OnCreate(Graphics& aGraphics)
{
	ASSERT_STR(!myGLProgram, "Pipeline already created!");
	myGLProgram = glCreateProgram();
}

bool PipelineGL::OnUpload(Graphics& aGraphics)
{
	Profiler::ScopedMark uploadMark("PipelineGL::OnUpload");

	ASSERT_STR(myGLProgram, "Pipeline missing!");

	const Pipeline* pipeline = myResHandle.Get<const Pipeline>();
	const size_t adapterCount = pipeline->GetAdapterCount();
	myAdapters.clear();
	myAdapters.reserve(adapterCount);
	for (size_t i = 0; i < adapterCount; i++)
	{
		myAdapters.push_back(&pipeline->GetAdapter(i));
	}

	// as part of hot reload, we might swap out the shaders,
	// so we gotta check if have changed the shaders we depend upon
	GLsizei shaderCount = static_cast<GLsizei>(pipeline->GetShaderCount());
	for (size_t i = 0; i < shaderCount; i++)
	{
		const std::string& shaderName = pipeline->GetShader(i);
		if (i >= myShaderNames.size())
		{
			Handle<Shader> shader = aGraphics.GetAssetTracker().GetOrCreate<Shader>(shaderName);
			myShaders.push_back(aGraphics.GetOrCreate(shader).Get<ShaderGL>());
			OnShaderAttach(*myShaders[i].Get());
			myShaderNames.push_back(shaderName);
		}
		else if (shaderName != myShaderNames[i])
		{
			Handle<Shader> shader = aGraphics.GetAssetTracker().GetOrCreate<Shader>(shaderName);
			OnShaderRemove(*myShaders[i].Get());
			myShaders[i] = aGraphics.GetOrCreate(shader).Get<ShaderGL>();
			OnShaderAttach(*myShaders[i].Get());
			myShaderNames[i] = shaderName;
		}
	}

	if (!AreDependenciesValid())
	{
		// Shaders haven't loaded yet - we gotta defer
		return false;
	}

	// Trim excess
	myShaders.resize(shaderCount);
	myShaderNames.resize(shaderCount);

	// To support multiple uploads for HotReload, we gotta
	// know what we can attach and what needs to be detached
	constexpr GLsizei kMaxShaders = 4;
	shaderCount = 0;
	GLuint shaders[kMaxShaders]{};
	bool wasFound[kMaxShaders]{};
	glGetAttachedShaders(myGLProgram, kMaxShaders, &shaderCount, shaders);

	for (Handle<ShaderGL> dependency : myShaders)
	{
		const ShaderGL* shader = dependency.Get<const ShaderGL>();
		const uint32_t id = shader->GetShaderId();

		auto iter = std::ranges::find(shaders, id);
		if (iter == std::end(shaders))
		{
			glAttachShader(myGLProgram, id);
		}
		else
		{
			const std::ptrdiff_t index = std::distance(std::begin(shaders), iter);
			wasFound[index] = true;
		}
	}

	// Before we link, drop any shaders that are no longer dependencies
	for (uint8_t i = 0; i < shaderCount; i++)
	{
		if (!wasFound[i])
		{
			glDetachShader(myGLProgram, shaders[i]);
		}
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
		// we've succesfully linked
#ifdef _DEBUG
		// validate descriptor matches UBOs
		if (!AreUBOsValid())
		{
			return false;
		}
#endif

		// Because samplers can't be part of the uniform blocks, 
		// they have to stay separate, which means we have to find them
		// and track them
		int uniformCount = 0;
		glGetProgramiv(myGLProgram, GL_ACTIVE_UNIFORMS, &uniformCount);
		ASSERT(uniformCount >= 0);
		mySamplerUniforms.clear();
		mySamplerTypes.clear();
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

	for (const Handle<ShaderGL> shaderHandle : myShaders)
	{
		if (shaderHandle->GetState() != State::Valid)
		{
			return false;
		}
	}

	return true;
}

void PipelineGL::OnShaderAttach(ShaderGL& aShader)
{
	aShader.AddDependent(this);
}

void PipelineGL::OnShaderRemove(ShaderGL& aShader)
{
	aShader.RemoveDependent(this);
}

#ifdef _DEBUG
bool PipelineGL::AreUBOsValid()
{
	const Pipeline* pipeline = myResHandle.Get<const Pipeline>();
	{
		const size_t totalAdapterCount = pipeline->GetAdapterCount()
			+ pipeline->GetGlobalAdapterCount();
		int uniformBlocks;
		glGetProgramInterfaceiv(myGLProgram, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES, &uniformBlocks);
		if (uniformBlocks != totalAdapterCount)
		{
			char errorMsg[200];
			Utils::StringFormat(errorMsg, "Mismatching adapter count, expecting {} got {}",
				totalAdapterCount, uniformBlocks);
			SetErrMsg(errorMsg);
			return false;
		}
	}

	// all good - time to resolve the UBOs
	auto CheckBindpoint = [this, pipeline](const UniformAdapter& anAdapter) 
	{
		const std::string_view uboName = anAdapter.GetName();
		const uint32_t uboIndex = glGetUniformBlockIndex(myGLProgram, uboName.data());
		ASSERT_STR(uboIndex != GL_INVALID_INDEX, "Got invalid index for {}!", uboName);

		int32_t declaredBinding;
		glGetActiveUniformBlockiv(myGLProgram, uboIndex, GL_UNIFORM_BLOCK_BINDING, &declaredBinding);

		if (declaredBinding != anAdapter.GetBindpoint())
		{
			char errorMsg[200];
			Utils::StringFormat(errorMsg, "Pipline {} had an invalid bindpoint for {}, supposed to be {}!",
				pipeline->GetName(), anAdapter.GetName(), anAdapter.GetBindpoint());
			SetErrMsg(errorMsg);
			return false;
		}
		return true;
	};
	auto CheckFormat = [this, pipeline](const UniformAdapter& anAdapter) 
	{
		// The driver returns uniforms in unspecified order while our 
		// descriptors are defined in the same order as uniforms
		// in a uniform block of a shader. So we have to cache and sort
		// the uniforms reported by the driver to ensure we're
		// comparing in the right order
		struct UniformInfo
		{
			int32_t myType;
			int32_t myOffset;
		};
		constexpr static uint8_t kMaxUniforms = 20;
		std::array<UniformInfo, kMaxUniforms> uniforms;

		const std::string_view uboName = anAdapter.GetName();
		const uint32_t blockInd = glGetUniformBlockIndex(myGLProgram, uboName.data());

		const uint32_t activeUniformCountProperty = GL_NUM_ACTIVE_VARIABLES;
		int32_t activeUniforms;
		glGetProgramResourceiv(myGLProgram, GL_UNIFORM_BLOCK, blockInd,
			1, &activeUniformCountProperty,
			1, nullptr, &activeUniforms
		);
		ASSERT_STR(activeUniforms < kMaxUniforms, "Please increase size of uniform cache!");

		const uint32_t uniformLocationsProperty = GL_ACTIVE_VARIABLES;
		std::array<int32_t, kMaxUniforms> uniformIDs;
		glGetProgramResourceiv(myGLProgram, GL_UNIFORM_BLOCK, blockInd,
			1, &uniformLocationsProperty,
			activeUniforms, nullptr, uniformIDs.data()
		);

		for (int32_t uniformInd = 0; uniformInd < activeUniforms; uniformInd++)
		{
			uint32_t uniformProperties[] = { GL_TYPE, GL_OFFSET };
			int32_t uniformInfo[2];
			glGetProgramResourceiv(myGLProgram, GL_UNIFORM, uniformIDs[uniformInd],
				2, uniformProperties,
				2, nullptr, uniformInfo
			);
			uniforms[uniformInd] = { uniformInfo[0], uniformInfo[1] };
		}

		std::sort(
			std::begin(uniforms),
			std::begin(uniforms) + activeUniforms,
			[](const UniformInfo& aLeft, const UniformInfo& aRight) {
			return aLeft.myOffset < aRight.myOffset;
		}
		);

		for (int32_t uniformInd = 0; uniformInd < activeUniforms; uniformInd++)
		{
			const Descriptor::UniformType declaredType = anAdapter.GetDescriptor().GetType(
				static_cast<uint32_t>(uniformInd)
			);
			bool missmatch = false;
			std::string_view typeName;
			// taken from https://www.khronos.org/registry/OpenGL/specs/gl/glspec45.core.pdf
			// page 109
			switch (uniforms[uniformInd].myType)
			{
			case GL_UNSIGNED_INT:
			case GL_INT:
				if (declaredType != Descriptor::UniformType::Int)
				{
					missmatch = true;
					typeName = "Int";
				}
				break;
			case GL_FLOAT:
				if (declaredType != Descriptor::UniformType::Float)
				{
					missmatch = true;
					typeName = "Float";
				}
				break;
			case GL_FLOAT_VEC2:
				if (declaredType != Descriptor::UniformType::Vec2)
				{
					missmatch = true;
					typeName = "Vec2";
				}
				break;
			case GL_FLOAT_VEC3:
				ASSERT_STR(false, "Vec3's are not supported by design - use vec4s!");
				break;
			case GL_FLOAT_VEC4:
				if (declaredType != Descriptor::UniformType::Vec4)
				{
					missmatch = true;
					typeName = "Vec4";
				}
				break;
			case GL_FLOAT_MAT4:
				if (declaredType != Descriptor::UniformType::Mat4)
				{
					missmatch = true;
					typeName = "Mat4";
				}
				break;
			default:
				ASSERT_STR(false, "Unrecognized uniform type!");
				return false;
			}

			if (missmatch)
			{
				char errorMsg[200];
				Utils::StringFormat(errorMsg,
					"For slot {} in uniform buffer {} expected %s but got {}",
					uniformInd,
					anAdapter.GetName(),
					Descriptor::UniformType::kNames[declaredType],
					typeName
				);
				SetErrMsg(errorMsg);
				return false;
			}
		}
		return true;
	};

	for (size_t i = 0; i < pipeline->GetAdapterCount(); i++)
	{
		const UniformAdapter& adapter = pipeline->GetAdapter(i);
		if (!CheckBindpoint(adapter) || !CheckFormat(adapter))
		{
			return false;
		}
	}

	for (size_t i = 0; i < pipeline->GetGlobalAdapterCount(); i++)
	{
		const UniformAdapter& adapter = pipeline->GetGlobalAdapter(i);
		if (!CheckBindpoint(adapter) || !CheckFormat(adapter))
		{
			return false;
		}
	}

	
	return true;
}
#endif