#include "Precomp.h"
#include "Shader.h"

#include <Core/Resources/Serializer.h>

Shader::Shader()
	: myType(IShader::Type::Invalid)
{
}

Shader::Shader(Resource::Id anId, std::string_view aPath)
	: Resource(anId, aPath)
	, myType(IShader::Type::Invalid)
{
}

void Shader::Serialize(Serializer& aSerializer)
{
	aSerializer.Serialize("myType", myType);

	std::string shaderSrcFile = GetPath();
	shaderSrcFile = shaderSrcFile.replace(shaderSrcFile.size() - 3, 3, "txt");
	aSerializer.SerializeExternal(shaderSrcFile, myFileContents, GetId());
}

void Shader::Upload(const char* aData, size_t aSize)
{
	myFileContents.resize(aSize);
	std::memcpy(myFileContents.data(), aData, aSize);
}