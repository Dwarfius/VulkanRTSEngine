#include "Precomp.h"
#include "Shader.h"

#include <Core/Resources/Serializer.h>

Shader::Shader()
	: myType(IShader::Type::Invalid)
{
}

Shader::Shader(Resource::Id anId, const std::string& aPath)
	: Resource(anId, aPath)
	, myType(IShader::Type::Invalid)
{
}

void Shader::Serialize(Serializer& aSerializer)
{
	aSerializer.Serialize("myType", myType);

	std::string shaderSrcFile = GetPath();
	shaderSrcFile = shaderSrcFile.replace(shaderSrcFile.size() - 3, 3, "txt");
	aSerializer.SerializeExternal(shaderSrcFile, myFileContents);
}