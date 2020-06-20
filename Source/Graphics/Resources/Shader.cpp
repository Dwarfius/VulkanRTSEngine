#include "Precomp.h"
#include "Shader.h"
#include <Core/File.h>

Shader::Shader()
	: myType(IShader::Type::Invalid)
{
}

Shader::Shader(Resource::Id anId, const std::string& aPath)
	: Resource(anId, aPath)
	, myType(IShader::Type::Invalid)
{
	if (aPath.length() > 4)
	{
		myType = DetermineType(aPath);
	}
}

void Shader::OnLoad(const File& aFile)
{
	// TODO: add a similar to .ppl file that contains paths to correct
	// shader files (GLSL/SPIR-V)
	// TODO: this does a copy - add a move from function to File
	// with safety-use asserts, or figure out alternative approach
	myFileContents = aFile.GetBuffer();
}

IShader::Type Shader::DetermineType(const std::string& aPath)
{
	const std::string ext = aPath.substr(aPath.length() - 4);
	if (ext == "vert")
	{
		return IShader::Type::Vertex;
	}
	else if (ext == "frag")
	{
		return IShader::Type::Fragment;
	}
	else if (ext == "tctr")
	{
		return IShader::Type::TessControl;
	}
	else if (ext == "tevl")
	{
		return IShader::Type::TessEval;
	}
	else if (ext == "geom")
	{
		return IShader::Type::Geometry;
	}
	else
	{
		ASSERT_STR(false, "Type not supported!");
		return IShader::Type::Invalid;
	}
}