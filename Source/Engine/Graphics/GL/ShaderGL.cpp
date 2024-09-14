#include "Precomp.h"
#include "ShaderGL.h"

#include <Graphics/Resources/Shader.h>
#include <Graphics/Graphics.h>
#include <Core/Profiler.h>

ShaderGL::ShaderGL()
	: myGLShader(0)
{
}

void ShaderGL::OnCreate(Graphics& aGraphics)
{
	ASSERT_STR(!myGLShader, "Shader already created!");

	const Shader* shader = myResHandle.Get<const Shader>();

	uint32_t glType = GL_VERTEX_SHADER;
	switch (shader->GetType())
	{
	case IShader::Type::Vertex:	glType = GL_VERTEX_SHADER; break;
	case IShader::Type::Fragment: glType = GL_FRAGMENT_SHADER; break;
	case IShader::Type::Geometry: glType = GL_GEOMETRY_SHADER; break;
	case IShader::Type::TessControl: glType = GL_TESS_CONTROL_SHADER; break;
	case IShader::Type::TessEval: glType = GL_TESS_EVALUATION_SHADER; break;
	case IShader::Type::Compute: glType = GL_COMPUTE_SHADER; break;
	default: ASSERT(false);
	}
	myGLShader = glCreateShader(glType);
}

bool ShaderGL::OnUpload(Graphics& aGraphics)
{
	Profiler::ScopedMark uploadMark("ShaderGL::OnUpload");

	ASSERT_STR(myGLShader, "Shader missing!");

	const Shader* shader = myResHandle.Get<const Shader>();
	const std::vector<char>& shaderBuffer = shader->GetBuffer();

	const char* dataPtrs[] = { shaderBuffer.data() };
	GLint dataSizes[] = { static_cast<GLint>(shaderBuffer.size()) };
	glShaderSource(myGLShader, 1, dataPtrs, dataSizes);

	glCompileShader(myGLShader);

	GLint isCompiled = 0;
	glGetShaderiv(myGLShader, GL_COMPILE_STATUS, &isCompiled);

	if (isCompiled != GL_TRUE)
	{
#ifdef _DEBUG
		int length = 0;
		glGetShaderiv(myGLShader, GL_INFO_LOG_LENGTH, &length);

		std::string errStr;
		errStr.resize(length);
		glGetShaderInfoLog(myGLShader, length, &length, &errStr[0]);

		std::string fullError = errStr;
		std::string_view lineNumView = errStr; // format is "smth-num(line-num) : rest"
		std::size_t end = lineNumView.find(") :");
		if (end != std::string_view::npos)
		{
			std::size_t start = lineNumView.substr(0, end).find('(', 1);
			if (start != std::string_view::npos)
			{
				fullError += "Context:\n";
				lineNumView = lineNumView.substr(start + 1, end - start - 1);
				uint32_t line = 0;
				auto [ptr, err] = std::from_chars(
					lineNumView.data(), lineNumView.data() + lineNumView.length(), line
				);
				if (err == std::errc{})
				{
					const uint32_t startLines = line - std::min(line, 3u);
					const uint32_t endLines = line + 3u;
					uint32_t found = 0;
					const std::string_view contents(
						shaderBuffer.data(), shaderBuffer.size()
					);
					size_t startIndex = 0;
					size_t endIndex = contents.find('\n');
					while (endIndex != std::string_view::npos)
					{
						found++;
						const std::string_view line = contents.substr(startIndex, endIndex - startIndex);

						if (found >= startLines)
						{
							fullError += '\n';
							fullError += std::to_string(found);
							fullError += ": ";
							fullError += line;
							if(found == endLines)
							{
								break;
							}
						}
						startIndex = endIndex + 1;
						endIndex = contents.find('\n', startIndex);
					}

				}
			}
		}

		SetErrMsg("Shader failed to compile, " + fullError);
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