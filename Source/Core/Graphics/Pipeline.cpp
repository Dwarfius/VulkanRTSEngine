#include "Precomp.h"
#include "Pipeline.h"

#include <nlohmann/json.hpp>

#include "Graphics.h"
#include "AssetTracker.h"

Pipeline::Pipeline(Resource::Id anId)
	: Pipeline(anId, "")
{
}

Pipeline::Pipeline(Resource::Id anId, const string& aPath)
	: Resource(anId, aPath)
	, myType(Type::Graphics)
{
}

void Pipeline::AddShader(Handle<Shader> aShader)
{
	ASSERT_STR(myType == Type::Compute && aShader->GetType() == Shader::Type::Compute
		|| myType == Type::Graphics && aShader->GetType() != Shader::Type::Compute,
		"Attempted to pass an incompatible shader to the pipeline!");
	myShaders.push_back(aShader);
	myDependencies.push_back(aShader.Get<Resource>());
}

void Pipeline::OnLoad(AssetTracker& anAssetTracker)
{
	string contents;
	if (!ReadFile("assets/pipelines/" + myPath, contents))
	{
		SetErrMsg("Failed to read file!");
		return;
	}

	using json = nlohmann::json;
	const json jsonObj = json::parse(contents, nullptr, false);
	{
		const json& typeHandle = jsonObj["type"];
		if (typeHandle.is_null())
		{
			SetErrMsg("Ppl missing type!");
			return;
		}

		myType = typeHandle.get<Type>();
		ASSERT_STR(myType == Type::Graphics, "Compute pipeline type not supported!");
	}

	// TODO: add conditional checking based on myType
	// vert shader...
	{
		const json& vertHandle = jsonObj["vert"];
		if (vertHandle.is_null())
		{
			SetErrMsg("Ppl missing vert!");
			return;
		}
		string vertShaderName = vertHandle.get<string>();
		Handle<Shader> vertShader = anAssetTracker.GetOrCreate<Shader>(vertShaderName);
		AddShader(vertShader);
	}

	// ...frag shader...
	{
		const json& fragHandle = jsonObj["frag"];
		if (fragHandle.is_null())
		{
			SetErrMsg("Ppl missing frag!");
			return;
		}
		string fragShaderName = fragHandle.get<string>();
		Handle<Shader> fragShader = anAssetTracker.GetOrCreate<Shader>(fragShaderName);
		AddShader(fragShader);
	}

	// ...descriptors
	{
		const json& descriptorArrayHandle = jsonObj["descriptors"];
		if (!descriptorArrayHandle.is_array())
		{
			SetErrMsg("Ppl missing descriptor array!");
			return;
		}
		size_t descriptorCount = descriptorArrayHandle.size();
		myDescriptors.resize(descriptorCount);
		for(size_t i=0; i<descriptorCount; i++)
		{
			const json& descriptorHandle = descriptorArrayHandle.at(i);
			if (!Descriptor::FromJSON(descriptorHandle, myDescriptors[i]))
			{
				SetErrMsg("Ppl has malformed descriptor!");
				return;
			}
		}
	}

	myState = State::PendingUpload;
}

void Pipeline::OnUpload(GPUResource* aGPURes)
{
	ASSERT_STR(myShaders.size() > 0, "Attempted to upload a pipeline with no shaders attached!");

	myGPUResource = aGPURes;

	CreateDescriptor createDesc;
	myGPUResource->Create(createDesc);

	vector<const GPUResource*> shaders;
	shaders.resize(myShaders.size());
	for (size_t i=0; i<myShaders.size(); i++)
	{
		shaders[i] = &myShaders[i]->GetGPUResource();
	}
	UploadDescriptor uploadDesc;
	uploadDesc.myShaders = shaders.data();
	uploadDesc.myShaderCount = myShaders.size();
	uploadDesc.myDescriptors = myDescriptors.data();
	uploadDesc.myDescriptorCount = myDescriptors.size();
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