#pragma once

#include <Core/Resources/Resource.h>
#include "../Interfaces/IShader.h"

// A base class for shaders
class Shader : public Resource, public IShader
{
public:
	constexpr static StaticString kExtension = ".shd";

	Shader();
	Shader(Resource::Id anId, const std::string& aPath);

	IShader::Type GetType() const { return myType; }
	void SetType(IShader::Type aType) { myType = aType; }

	const std::vector<char>& GetBuffer() const { return myFileContents; }

protected:
	IShader::Type myType;
	std::vector<char> myFileContents;

private:
	void Serialize(Serializer& aSerializer) final;
};