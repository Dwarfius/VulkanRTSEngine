#include "Precomp.h"
#include "ShaderGL.h"

#include <Graphics/Resources/Shader.h>
#include <Graphics/Graphics.h>

ShaderGL::ShaderGL()
	: myGLShader(0)
{
}

void ShaderGL::OnCreate(Graphics& aGraphics)
{
	ASSERT_STR(!myGLShader, "Shader already created!");

	const Shader* shader = myResHandle.Get<const Shader>();

	uint32_t glType;
	switch (shader->GetType())
	{
	case IShader::Type::Vertex:		glType = GL_VERTEX_SHADER; break;
	case IShader::Type::Fragment:	glType = GL_FRAGMENT_SHADER; break;
	case IShader::Type::Geometry:	glType = GL_GEOMETRY_SHADER; break;
	case IShader::Type::TessControl: glType = GL_TESS_CONTROL_SHADER; break;
	case IShader::Type::TessEval:	glType = GL_TESS_EVALUATION_SHADER; break;
	case IShader::Type::Compute:		glType = GL_COMPUTE_SHADER; break;
	default: ASSERT(false);
	}
	myGLShader = glCreateShader(glType);
}

bool ShaderGL::OnUpload(Graphics& aGraphics)
{
	ASSERT_STR(myGLShader, "Shader missing!");

	const Shader* shader = myResHandle.Get<const Shader>();
	const std::string& shaderBuffer = shader->GetBuffer();

	const char* dataPtrs[] = {						shaderBuffer.data()   };
	int dataSizes[] = {			static_cast<GLint>(	shaderBuffer.size() ) };
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

		SetErrMsg("Shader failed to compile" + errStr);
#endif
		return false;
	}
	return true;
}

void ShaderGL::OnUnload(Graphics& aGraphics)
{
	ASSERT_STR(myGLShader, "Empty shader detected!");
	glDeleteShader(myGLShader);
	myGLShader = 0;
}