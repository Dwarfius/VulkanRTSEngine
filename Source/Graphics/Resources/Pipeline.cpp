#include "Precomp.h"
#include "Pipeline.h"

#include <nlohmann/json.hpp>

#include "../Graphics.h"
#include "../AssetTracker.h"
#include <Core/File.h>

Pipeline::Pipeline()
	: myType(IPipeline::Type::Graphics)
{
}

Pipeline::Pipeline(Resource::Id anId, const std::string& aPath)
	: Resource(anId, kDir.CStr() + aPath)
	, myType(IPipeline::Type::Graphics)
{
}

void Pipeline::AddShader(Handle<Shader> aShader)
{
	ASSERT_STR(myType == IPipeline::Type::Compute && aShader->GetType() == IShader::Type::Compute
		|| myType == IPipeline::Type::Graphics && aShader->GetType() != IShader::Type::Compute,
		"Attempted to pass an incompatible shader to the pipeline!");
	myShaders.push_back(aShader);
	myDependencies.push_back(aShader.Get<Resource>());
}

void Pipeline::OnLoad(AssetTracker& anAssetTracker, const File& aFile)
{
	using json = nlohmann::json;

	const json jsonObj = json::parse(aFile.GetBuffer(), nullptr, false);
	{
		const json& typeHandle = jsonObj["type"];
		if (typeHandle.is_null())
		{
			SetErrMsg("Ppl missing type!");
			return;
		}

		myType = typeHandle.get<IPipeline::Type>();
		ASSERT_STR(myType == IPipeline::Type::Graphics, "Compute pipeline type not supported!");
	}

	{
		const json& shaderArrayHandle = jsonObj["shaders"];
		if (!shaderArrayHandle.is_array())
		{
			SetErrMsg("Ppl missing shader array!");
			return;
		}

		bool foundVert = false;
		bool foundFrag = false;
		const size_t shaderCount = shaderArrayHandle.size();
		for (size_t i = 0; i < shaderCount; i++)
		{
			const json& shaderHandle = shaderArrayHandle.at(i);
			std::string shaderName = shaderHandle.get<std::string>();
			Handle<Shader> shader = anAssetTracker.GetOrCreate<Shader>(shaderName);
			foundVert |= shader->GetType() == IShader::Type::Vertex;
			foundFrag |= shader->GetType() == IShader::Type::Fragment;
			AddShader(shader);
		}

		if (!foundVert || !foundFrag)
		{
			SetErrMsg("Ppl missing vert of frag shaders!");
			return;
		}
	}

	// ...descriptors
	{
		const json& descriptorArrayHandle = jsonObj["descriptors"];
		if (!descriptorArrayHandle.is_array())
		{
			SetErrMsg("Ppl missing descriptor array!");
			return;
		}
		const size_t descriptorCount = descriptorArrayHandle.size();
		myDescriptors.resize(descriptorCount);
		for (size_t i = 0; i < descriptorCount; i++)
		{
			const json& descriptorHandle = descriptorArrayHandle.at(i);
			if (!Descriptor::FromJSON(descriptorHandle, myDescriptors[i]))
			{
				SetErrMsg("Ppl has malformed descriptor!");
				return;
			}
		}
	}
}