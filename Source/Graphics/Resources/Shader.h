#pragma once

#include <Core/Resources/Resource.h>
#include "../Interfaces/IShader.h"

// A base class for shaders
class Shader : public Resource, public IShader
{
public:
	constexpr static StaticString kExtension = ".shd";

	Shader();
	Shader(Resource::Id anId, std::string_view aPath);

	IShader::Type GetType() const { return myType; }
	void SetType(IShader::Type aType) { myType = aType; }

	const std::vector<char>& GetBuffer() const { return myFileContents; }

	void Upload(const char* aData, size_t aSize);
protected:
	IShader::Type myType;
	std::vector<char> myFileContents;

private:
	void Serialize(Serializer& aSerializer) final;
};