#include "Precomp.h"
#include "Shader.h"

Shader::Shader(Resource::Id anId)
	: Shader(anId, "")
{
}

Shader::Shader(Resource::Id anId, const string& aPath)
	: Resource(anId, aPath)
	, myType(Type::Vertex)
{
}

void Shader::Load()
{
	// first determine the type of shader based on file extension
	// currently only vertex and fragment shaders are supported
	string ext = myPath.substr(myPath.length() - 4);
	if (ext == "vert")
	{
		myType = Type::Vertex;
	}
	else if (ext == "frag")
	{
		myType = Type::Fragment;
	}
	else
	{
		ASSERT_STR(false, "Type not supported!");
	}

	bool success = ReadFile(myPath, myFileContents);
	if (!success)
	{
		SetErrMsg("Failed to read file!");
		return;
	}

	myState = State::PendingUpload;
}

void Shader::Upload(GPUResource* aGPUResource)
{
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