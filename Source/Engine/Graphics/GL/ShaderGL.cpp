#include "Precomp.h"
#include "ShaderGL.h"

ShaderGL::ShaderGL()
	: GPUResource()
	, myGLShader(0)
{
}

ShaderGL::~ShaderGL()
{
	if (myGLShader)
	{
		Unload();
	}
}

void ShaderGL::Create(any aDescriptor)
{
	ASSERT_STR(!myGLShader, "Shader already created!");

	const Shader::CreateDescriptor& desc = 
		any_cast<const Shader::CreateDescriptor&>(aDescriptor);

	uint32_t glType;
	switch (desc.myType)
	{
	case Shader::Type::Vertex:		glType = GL_VERTEX_SHADER; break;
	case Shader::Type::Fragment:	glType = GL_FRAGMENT_SHADER; break;
	case Shader::Type::Geometry:	glType = GL_GEOMETRY_SHADER; break;
	case Shader::Type::TessControl: glType = GL_TESS_CONTROL_SHADER; break;
	case Shader::Type::TessEval:	glType = GL_TESS_EVALUATION_SHADER; break;
	case Shader::Type::Compute:		glType = GL_COMPUTE_SHADER; break;
	}
	myGLShader = glCreateShader(glType);
}

bool ShaderGL::Upload(any aDescriptor)
{
	const Shader::UploadDescriptor& desc = 
		any_cast<const Shader::UploadDescriptor&>(aDescriptor);

	ASSERT_STR(myGLShader, "Shader missing!");

	const char* dataPtrs[] = {						desc.myFileContents.data()   };
	int dataSizes[] = {			static_cast<GLint>(	desc.myFileContents.size() ) };
	glShaderSource(myGLShader, 1, dataPtrs, dataSizes);

	glCompileShader(myGLShader);

	GLint isCompiled = 0;
	glGetShaderiv(myGLShader, GL_COMPILE_STATUS, &isCompiled);

	if (isCompiled != GL_TRUE)
	{
#ifdef _DEBUG
		int length = 0;
		glGetShaderiv(myGLShader, GL_INFO_LOG_LENGTH, &length);

		string errStr;
		errStr.resize(length);
		glGetShaderInfoLog(myGLShader, length, &length, &errStr[0]);

		myErrorMsg = "Shader failed to compile" + errStr;
#endif
		return false;
	}

	return true;
}

void ShaderGL::Unload()
{
	ASSERT_STR(myGLShader, "Empty shader detected!");
	glDeleteShader(myGLShader);
	myGLShader = 0;
}