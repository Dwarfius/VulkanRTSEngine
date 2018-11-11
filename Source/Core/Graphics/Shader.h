#pragma once

#include "Resource.h"

// A base class for shaders
class Shader : public Resource
{
public:
	enum class Type
	{
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
		string_view myFileContents;
	};

public:
	Shader(Resource::Id anId);
	Shader(Resource::Id anId, const string& aPath);

	Resource::Type GetResType() const { return Resource::Type::Shader; }

	Type GetType() const { return myType; }

protected:
	Type myType;
	string myFileContents;

private:
	void OnLoad() override;
	void OnUpload(GPUResource* aGPUResource) override;
};