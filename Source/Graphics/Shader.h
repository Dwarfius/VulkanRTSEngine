#pragma once

#include "Resource.h"

// A base class for shaders
class Shader : public Resource
{
public:
	enum class Type
	{
		Invalid,
		Vertex,
		Fragment,
		Geometry,
		TessControl,
		TessEval,
		Compute
	};

	struct CreateDescriptor
	{
		Type myType;
	};

	struct UploadDescriptor
	{
		std::string_view myFileContents;
	};

public:
	Shader(Resource::Id anId);
	Shader(Resource::Id anId, const std::string& aPath);

	Resource::Type GetResType() const { return Resource::Type::Shader; }

	Type GetType() const { return myType; }

protected:
	Type myType;
	std::string myFileContents;

	static Type DetermineType(const std::string& aPath);

private:
	void OnLoad(AssetTracker& anAssetTracker) override;
	void OnUpload(GPUResource* aGPUResource) override;
};