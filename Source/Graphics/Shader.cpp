#include "Precomp.h"
#include "Shader.h"

Shader::Shader(Resource::Id anId)
	: Shader(anId, "")
{
}

Shader::Shader(Resource::Id anId, const std::string& aPath)
	: Resource(anId, aPath)
	, myType(Type::Invalid)
{
	if (aPath.length() > 4)
	{
		myType = DetermineType(aPath);
	}
}

void Shader::OnLoad(AssetTracker& anAssetTracker)
{
	bool success = ReadFile(myPath, myFileContents);
	if (!success)
	{
		SetErrMsg("Failed to read file!");
		return;
	}

	// TODO: add a similar to .ppl file that contains paths to correct
	// shader files (GLSL/SPIR-V)

	myState = State::PendingUpload;
}

void Shader::OnUpload(GPUResource* aGPUResource)
{
	ASSERT_STR(myType != Type::Invalid, "This resource wasn't setup correctly for uploading!");

	myGPUResource = aGPUResource;
	
	CreateDescriptor createDesc;
	createDesc.myType = myType;
	myGPUResource->Create(createDesc);

	UploadDescriptor uploadDesc;
	uploadDesc.myFileContents = myFileContents;
	bool success = myGPUResource->Upload(uploadDesc);

	if (success)
	{
		myState = State::Ready;
	}
	else
	{
		SetErrMsg(myGPUResource->GetErrorMsg());
	}
}

Shader::Type Shader::DetermineType(const std::string& aPath)
{
	const std::string ext = aPath.substr(aPath.length() - 4);
	if (ext == "vert")
	{
		return Type::Vertex;
	}
	else if (ext == "frag")
	{
		return Type::Fragment;
	}
	else if (ext == "tctr")
	{
		return Type::TessControl;
	}
	else if (ext == "tevl")
	{
		return Type::TessEval;
	}
	else if (ext == "geom")
	{
		return Type::Geometry;
	}
	else
	{
		ASSERT_STR(false, "Type not supported!");
		return Type::Invalid;
	}
}