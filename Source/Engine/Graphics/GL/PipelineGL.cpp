#include "Precomp.h"
#include "PipelineGL.h"

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
}

void PipelineGL::Create(any aDescriptor)
{
	ASSERT_STR(!myGLProgram, "Pipeline already created!");
	myGLProgram = glCreateProgram();
}

bool PipelineGL::Upload(any aDescriptor)
{
	Pipeline::UploadDescriptor desc = any_cast<Pipeline::UploadDescriptor>(aDescriptor);

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
	return linked;

	// create a descriptor for the pipeline
	// !!!Stopped here!!!
	// TODO: rewrite this to use actual UBOs, also need to nail down
	// Pipeline <=> UBO relationship, specifically, need to figure out
	// how to allow access to the UBO buffer
	/*uint32_t programId = pipeline.GetProgramId();

	//going to iterate through every uniform and cache info about it
	GLint uniformCount = 0;
	glGetProgramiv(programId, GL_ACTIVE_UNIFORMS, &uniformCount);

	const int maxLength = 100;
	char nameChars[maxLength];
	for (int i = 0; i < uniformCount; i++)
	{
		GLenum type = 0;
		GLsizei nameLength = 0;
		GLsizei uniSize = 0;
		glGetActiveUniform(programId, i, maxLength, &nameLength, &uniSize, &type, nameChars);
		string name(nameChars, nameLength);
		GLint loc = glGetUniformLocation(programId, name.c_str());

		// converting to API independent format
		Shader::UniformType uniformType;
		switch (type)
		{
		case GL_FLOAT:		uniformType = Shader::UniformType::Float;	break;
		case GL_FLOAT_VEC2:	uniformType = Shader::UniformType::Vec2;	break;
		case GL_FLOAT_VEC3: uniformType = Shader::UniformType::Vec3;	break;
		case GL_FLOAT_VEC4: uniformType = Shader::UniformType::Vec4;	break;
		case GL_FLOAT_MAT4: uniformType = Shader::UniformType::Mat4;	break;
		default:			uniformType = Shader::UniformType::Int;		break;
		}

		Shader::BindPoint bindPoint{ loc, uniformType };
		pipeline.myUniforms[name] = bindPoint;
	}*/
}

void PipelineGL::Unload()
{
	ASSERT_STR(myGLProgram, "Empty pipeline detected!");
	glDeleteProgram(myGLProgram);
}