#pragma once

#include "../Resource.h"
#include "../Interfaces/IShader.h"

// A base class for shaders
class Shader : public Resource, public IShader
{
public:
	Shader();
	Shader(Resource::Id anId, const std::string& aPath);

	Resource::Type GetResType() const { return Resource::Type::Shader; }

	IShader::Type GetType() const { return myType; }
	const std::string& GetBuffer() const { return myFileContents; }

protected:
	IShader::Type myType;
	std::string myFileContents;

	static IShader::Type DetermineType(const std::string& aPath);

private:
	void OnLoad(AssetTracker& anAssetTracker, const File& aFile) override;
};