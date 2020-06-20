#pragma once

#include <Core/Resources/Resource.h>
#include "../Interfaces/IShader.h"

// A base class for shaders
class Shader : public Resource, public IShader
{
public:
	static constexpr StaticString kDir = Resource::AssetsFolder + "shaders/";

	Shader();
	Shader(Resource::Id anId, const std::string& aPath);

	IShader::Type GetType() const { return myType; }
	const std::string& GetBuffer() const { return myFileContents; }

protected:
	IShader::Type myType;
	std::string myFileContents;

	static IShader::Type DetermineType(const std::string& aPath);

private:
	bool UsesDescriptor() const override final { return false; }
	void OnLoad(const File& aFile) override;
};